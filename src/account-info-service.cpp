#include <QTimer>

#include "account-info-service.h"
#include "account-mgr.h"
#include "api/api-error.h"
#include "api/requests.h"
#include "seafile-applet.h"

namespace
{
// const int kRefreshInterval = 3 * 60 * 1000; // 3 min
}

SINGLETON_IMPL(AccountInfoService)

AccountInfoService::AccountInfoService(QObject* parent)
    : QObject(parent), request_(NULL)
{
    refresh_timer_ = new QTimer(this);
    connect(refresh_timer_, SIGNAL(timeout()), this, SLOT(refresh()));
    // refresh();
}

void AccountInfoService::start()
{
    // refresh_timer_->start(kRefreshInterval);
}

void AccountInfoService::stop()
{
    refresh_timer_->stop();
}

void AccountInfoService::refresh()
{
    const Account account = seafApplet->accountManager()->currentAccount();
    if (!account.isValid()) {
        return;
    }
    if (request_) {
        request_->deleteLater();
        request_ = NULL;
    }

    request_ = new FetchAccountInfoRequest(account);
    connect(request_, SIGNAL(success(const AccountInfo&)), this,
            SLOT(onFetchAccountInfoSuccess(const AccountInfo&)));
    connect(request_, SIGNAL(failed(const ApiError&)), this,
            SLOT(onFetchAccountInfoFailed()));
    request_->send();
}


void AccountInfoService::onFetchAccountInfoSuccess(const AccountInfo& info)
{
    if(request_ == NULL) {
        return;
    }
    seafApplet->accountManager()->updateAccountInfo(request_->account(), info);
    request_->deleteLater();
    request_ = NULL;
}

void AccountInfoService::onFetchAccountInfoFailed()
{
    if(request_ == NULL) {
        return;
    }
    request_->deleteLater();
    request_ = NULL;
}
