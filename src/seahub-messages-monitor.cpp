#include <QToolButton>
#include <QTimer>
#include <QUrl>
#include <QDesktopServices>

#include "QtAwesome.h"
#include "ui/cloud-view.h"
#include "api/requests.h"
#include "seahub-messages-monitor.h"

namespace {

const int kRefreshSeahubMessagesInterval = 1000 * 60; // 1min
const char *kMessagesUrl = "/notification/list/";

} // namespace

SeahubMessagesMonitor::SeahubMessagesMonitor(CloudView *cloud_view, QObject *parent)
    : QObject(parent),
      cloud_view_(cloud_view),
      unread_count_(0),
      req_(0)
{
    btn_ = cloud_view->seahubMessagesBtn();

    btn_->setVisible(false);

    resetStatus();

    connect(btn_, SIGNAL(clicked()), this, SLOT(onBtnClicked()));

    refresh_timer_ = new QTimer(this);
    connect(refresh_timer_, SIGNAL(timeout()), this, SLOT(refresh()));

    refresh_timer_->start(kRefreshSeahubMessagesInterval);
}

void SeahubMessagesMonitor::resetStatus()
{
    btn_->setVisible(false);
}

void SeahubMessagesMonitor::refresh()
{
    const Account& account = cloud_view_->currentAccount();
    if (!account.isValid()) {
        resetStatus();
        return;
    }

    req_ = new GetUnseenSeahubMessagesRequest(account);

    connect(req_, SIGNAL(success(int)),
            this, SLOT(onRequestSuccess(int)));

    req_->send();
}

void SeahubMessagesMonitor::onRequestSuccess(int count)
{
    QString tip;
    unread_count_ = count;

    if (count == 0) {
        resetStatus();
        return;
    }

    tip = tr("You have %n message(s)", "", count);

    btn_->setVisible(true);
    btn_->setToolTip(tip);
    btn_->setIcon(QIcon(":/images/bell-orange.png"));
}

void SeahubMessagesMonitor::onBtnClicked()
{
    const Account& account = cloud_view_->currentAccount();

    QUrl url = account.serverUrl;
    url.setPath(url.path() + kMessagesUrl);

    QDesktopServices::openUrl(url);

    resetStatus();
}
