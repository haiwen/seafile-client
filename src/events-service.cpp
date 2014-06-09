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
        singleton_ = new EventsService;
    }

    return singleton_;
}

EventsService::EventsService(QObject *parent)
    : QObject(parent)
{
    refresh_timer_ = new QTimer(this);
    connect(refresh_timer_, SIGNAL(timeout()), this, SLOT(refresh()));
    get_events_req_ = NULL;
    in_refresh_ = false;
    more_offset_ = -1;
}

void EventsService::start()
{
    refresh_timer_->start(kRefreshEventsInterval);
    refresh();
}

void EventsService::refresh()
{
    if (in_refresh_) {
        return;
    }

    AccountManager *account_mgr = seafApplet->accountManager();

    const Account& account = seafApplet->accountManager()->currentAccount();
    if (!account.isValid()) {
        in_refresh_ = false;
        return;
    }

    in_refresh_ = true;

    if (get_events_req_) {
        delete get_events_req_;
    }

    get_events_req_ = new GetEventsRequest(account, more_offset_);

    connect(get_events_req_, SIGNAL(success(const std::vector<SeafEvent>&, int)),
            this, SLOT(onRefreshSuccess(const std::vector<SeafEvent>&, int)));

    connect(get_events_req_, SIGNAL(failed(const ApiError&)),
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

    events_ = events;

    bool is_loading_more = more_offset_ > 0;
    bool has_more = new_offset > 0;

    more_offset_ = new_offset;

    emit refreshSuccess(events, is_loading_more, has_more);
}

void EventsService::onRefreshFailed(const ApiError& error)
{
    in_refresh_ = false;

    emit refreshFailed(error);
}

void EventsService::refresh(bool force)
{
    if (force) {
        more_offset_ = -1;
        in_refresh_ = false;
    }

    refresh();
}
