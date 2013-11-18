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
const char *kPersonalMessagesPath = "/message/list/";


} // namespace

SeahubMessagesMonitor::SeahubMessagesMonitor(CloudView *cloud_view, QObject *parent)
    : QObject(parent),
      cloud_view_(cloud_view),
      group_messages_(0),
      personal_messages_(0),
      req_(0)
{
    btn_ = cloud_view->seahubMessagesBtn();

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

    req_ = new GetSeahubMessagesRequest(account);

    connect(req_, SIGNAL(success(int, int)),
            this, SLOT(onRequestSuccess(int, int)));

    req_->send();
}

void SeahubMessagesMonitor::onRequestSuccess(int group_messages, int personal_messages)
{
    QString tip;
    group_messages_ = group_messages;
    personal_messages_ = personal_messages;

    if (group_messages == 0 && personal_messages == 0) {
        resetStatus();
        return;
    }

    if (group_messages > 0) {
        tip += tr("You have %n group message(s)", "", group_messages);

    } else if (personal_messages > 0) {
        tip += tr("You have %n personal message(s)", "", personal_messages);
    }

    btn_->setVisible(true);
    btn_->setToolTip(tip);
    btn_->setIcon(QIcon(":/images/bell-orange.png"));
}

void SeahubMessagesMonitor::onBtnClicked()
{
    if (group_messages_ == 0 && personal_messages_ == 0) {
        return;
    }

    const Account& account = cloud_view_->currentAccount();
    QString path = group_messages_ > 0 ? "" : kPersonalMessagesPath;

    QUrl url = account.serverUrl;
    url.setPath(url.path() + path);

    QDesktopServices::openUrl(url);

    resetStatus();
}
