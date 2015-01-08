#include <QTimer>

#include "seafile-applet.h"
#include "account-mgr.h"
#include "api/requests.h"
#include "events-service.h"

namespace {

const int kRefreshEventsInterval = 1000 * 60 * 5; // 5 min

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
    refresh_timer_ = new QTimer(this);
    connect(refresh_timer_, SIGNAL(timeout()), this, SLOT(refresh()));
    in_refresh_ = false;
    more_offset_ = -1;
}

void EventsService::start()
{
    refresh_timer_->start(kRefreshEventsInterval);
}

void EventsService::stop()
{
    refresh_timer_->stop();
}

void EventsService::refresh()
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

    get_events_req_.reset(new GetEventsRequest(account, more_offset_));

    connect(get_events_req_.data(),
            SIGNAL(success(const std::vector<SeafEvent>&, int)),
            this, SLOT(onRefreshSuccess(const std::vector<SeafEvent>&, int)));

    connect(get_events_req_.data(),
            SIGNAL(failed(const ApiError&)),
            this, SLOT(onRefreshFailed(const ApiError&)));

    get_events_req_->send();
}

void EventsService::loadMore()
{
    refresh();
}

void EventsService::onRefreshSuccess(const std::vector<SeafEvent>& events, int new_offset)
{
    in_refresh_ = false;

    // XXX: uncomment this when we need "load more events feature"
    /*
    const std::vector<SeafEvent> new_events = handleEventsOffset(events);

    bool is_loading_more = more_offset_ > 0;
    bool has_more = new_offset > 0;

    more_offset_ = new_offset;

    emit refreshSuccess(new_events, is_loading_more, has_more);
    */

    emit refreshSuccess(events, false, false);
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
        events_.clear();
        more_offset_ = -1;
        in_refresh_ = false;
    }

    refresh();
}
