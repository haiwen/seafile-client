#include <QTimer>
#include <QUrl>
#include <QDesktopServices>

#include "account-mgr.h"
#include "seafile-applet.h"
#include "api/requests.h"
#include "seahub-notifications-monitor.h"

namespace {

const int kRefreshSeahubMessagesInterval = 1000 * 60; // 1 min
const char *kNotificationsUrl = "/notification/list/";

} // namespace


SeahubNotificationsMonitor* SeahubNotificationsMonitor::singleton_;

SeahubNotificationsMonitor* SeahubNotificationsMonitor::instance()
{
    if (singleton_ == NULL) {
        singleton_ = new SeahubNotificationsMonitor;
    }

    return singleton_;
}

SeahubNotificationsMonitor::SeahubNotificationsMonitor(QObject *parent)
    : QObject(parent),
      check_messages_req_(0),
      in_refresh_(false),
      unread_count_(0)
{
}

void SeahubNotificationsMonitor::start()
{
    resetStatus();

    refresh_timer_ = new QTimer(this);
    connect(refresh_timer_, SIGNAL(timeout()), this, SLOT(refresh()));

    refresh_timer_->start(kRefreshSeahubMessagesInterval);

    connect(seafApplet->accountManager(), SIGNAL(accountsChanged()),
            this, SLOT(onAccountChanged()));
    refresh();
}

void SeahubNotificationsMonitor::onAccountChanged()
{
    refresh(true);
}

void SeahubNotificationsMonitor::resetStatus()
{
    setUnreadNotificationsCount(0);
}

void SeahubNotificationsMonitor::refresh()
{
    if (in_refresh_) {
        return;
    }

    const Account& account = seafApplet->accountManager()->currentAccount();
    if (!account.isValid()) {
        resetStatus();
        return;
    }

    in_refresh_ = true;

    if (check_messages_req_) {
        delete check_messages_req_;
    }

    check_messages_req_ = new GetUnseenSeahubNotificationsRequest(account);

    connect(check_messages_req_, SIGNAL(success(int)),
            this, SLOT(onRequestSuccess(int)));
    connect(check_messages_req_, SIGNAL(failed(const ApiError&)),
            this, SLOT(onRequestFailed(const ApiError&)));

    check_messages_req_->send();
}

void SeahubNotificationsMonitor::onRequestFailed(const ApiError& error)
{
    in_refresh_ = false;
}

void SeahubNotificationsMonitor::onRequestSuccess(int count)
{
    in_refresh_ = false;
    setUnreadNotificationsCount(count);
}

void SeahubNotificationsMonitor::refresh(bool force)
{
    if (force) {
        resetStatus();
        in_refresh_ = false;
    }

    refresh();
}

void SeahubNotificationsMonitor::openNotificationsPageInBrowser()
{
    const Account& account = seafApplet->accountManager()->currentAccount();
    if (!account.isValid()) {
        return;
    }

    QUrl url = account.serverUrl;

    url.setPath(url.path() + kNotificationsUrl);

    QDesktopServices::openUrl(url);

    resetStatus();
}

void SeahubNotificationsMonitor::setUnreadNotificationsCount(int count)
{
    if (unread_count_ != count) {
        unread_count_ = count;
        emit notificationsChanged();
    }
}
