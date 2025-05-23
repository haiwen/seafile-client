#include "seafile-applet.h"
#include "account-mgr.h"
#include "api/requests.h"
#include "events-service.h"

namespace {

const int kEventsPerPageForNewApi = 25;
} // namespace

EventsService* EventsService::singleton_;

EventsService* EventsService::instance()
{
    if (singleton_ == NULL) {
        static EventsService instance;
        singleton_ = &instance;
    }

    return singleton_;
}

EventsService::EventsService(QObject *parent)
    : QObject(parent)
{
    get_file_activities_req_ = NULL;
    next_ = -1;
    in_refresh_ = false;
}

void EventsService::start()
{
}

void EventsService::stop()
{
}

void EventsService::refresh()
{
    if (seafApplet->accountManager()->currentAccount().isPro()) {
        sendRequest(false);
    }
}

void EventsService::sendRequest(bool is_load_more)
{
    if (in_refresh_) {
        return;
    }

    const Account& account = seafApplet->accountManager()->currentAccount();
    if (!account.isValid()) {
        in_refresh_ = false;
        return;
    }

    in_refresh_ = true;

    if (get_file_activities_req_) {
        get_file_activities_req_->deleteLater();
    }

    if (!is_load_more) {
        events_.clear();
        next_ = 1;
    } else {
        ++next_;
    }

    get_file_activities_req_ = new GetEventsRequestV2(account, next_);
    connect(get_file_activities_req_, SIGNAL(success(const std::vector<SeafEvent>&)), this, SLOT(onRefreshSuccessV2(const std::vector<SeafEvent>&)));
    connect(get_file_activities_req_, SIGNAL(failed(const ApiError&)), this, SLOT(onRefreshFailed(const ApiError&)));
    get_file_activities_req_->send();
}

void EventsService::loadMore()
{
    sendRequest(true);
}

void EventsService::onRefreshSuccess(const std::vector<SeafEvent>& events, int new_offset)
{
    in_refresh_ = false;

    const std::vector<SeafEvent> new_events = handleEventsOffset(events);

    bool is_loading_more = next_ > 0;
    bool has_more = new_offset > 0;
    next_ = new_offset;
    emit refreshSuccess(new_events, is_loading_more, has_more);
}

void EventsService::onRefreshSuccessV2(const std::vector<SeafEvent>& events)
{
    in_refresh_ = false;

    const std::vector<SeafEvent> new_events = handleEventsOffset(events);

    bool has_more = events.size() == kEventsPerPageForNewApi;
    bool is_loading_more = next_ > 1;
    if (!has_more) {
        next_ = -1;
    }

    emit refreshSuccess(new_events, is_loading_more, has_more);
}

// We use the "offset" param as the starting point of loading more events, but
// if there are new events on the server, the offset would be inaccurate.
const std::vector<SeafEvent>
EventsService::handleEventsOffset(const std::vector<SeafEvent>& new_events)
{
    if (events_.empty()) {
        events_ = new_events;
        return events_;
    }

    const SeafEvent& last = events_[events_.size() - 1];

    int i = 0, n = new_events.size();

    for (i = 0; i < n; i++) {
        const SeafEvent& event = new_events[i];
        if (event.timestamp < last.timestamp) {
            break;
        } else if (event.commit_id == last.commit_id) {
            continue;
        } else {
            continue;
        }
    }

    std::vector<SeafEvent> ret;

    while (i < n) {
        SeafEvent event = new_events[i++];
        events_.push_back(event);
        ret.push_back(event);
    }

    return ret;
}

void EventsService::onRefreshFailed(const ApiError& error)
{
    in_refresh_ = false;

    emit refreshFailed(error);
}

void EventsService::refresh(bool force)
{
    if (force) {
        in_refresh_ = false;
    }

    refresh();
}
