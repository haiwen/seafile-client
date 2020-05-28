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
    : QObject(parent)
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

    FetchAccountInfoRequest* fetch_account_info_request = new FetchAccountInfoRequest(account);
    connect(fetch_account_info_request, SIGNAL(success(const AccountInfo&)), this,
            SLOT(onFetchAccountInfoSuccess(const AccountInfo&)));
    connect(fetch_account_info_request, SIGNAL(failed(const ApiError&)), this,
            SLOT(onFetchAccountInfoFailed()));
    fetch_account_info_request->send();
}


void AccountInfoService::onFetchAccountInfoSuccess(const AccountInfo& info)
{
    FetchAccountInfoRequest *req = (FetchAccountInfoRequest *)QObject::sender();
    seafApplet->accountManager()->updateAccountInfo(req->account(), info);
    req->deleteLater();
    req = NULL;
}

void AccountInfoService::onFetchAccountInfoFailed()
{
    FetchAccountInfoRequest *req = (FetchAccountInfoRequest *)QObject::sender();
    req->deleteLater();
    req = NULL;
}
