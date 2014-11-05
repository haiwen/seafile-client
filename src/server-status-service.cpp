#include <QTimer>

#include "server-status-service.h"
#include "seafile-applet.h"
#include "account-mgr.h"
#include "api/api-error.h"
#include "api/requests.h"

namespace {

const int kRefreshInternval = 3 * 60 * 1000; // 3 min

}

SINGLETON_IMPL(ServerStatusService)

ServerStatusService::ServerStatusService(QObject *parent)
    : QObject(parent)
{
    refresh_timer_ = new QTimer(this);
    connect(refresh_timer_, SIGNAL(timeout()),
            this, SLOT(refresh()));
    refresh();
}

void ServerStatusService::start()
{
    refresh_timer_->start(kRefreshInternval);
}

void ServerStatusService::stop()
{
    refresh_timer_->stop();
}

void ServerStatusService::refresh()
{
    const std::vector<Account>& accounts = seafApplet->accountManager()->accounts();
    for (int i = 0; i < accounts.size(); i++) {
        if (requests_.contains(accounts[i].serverUrl.host())) {
            return;
        }
        pingServer(accounts[i].serverUrl);
    }
}

void ServerStatusService::pingServer(const QUrl& url)
{
    PingServerRequest *req = new PingServerRequest(url);
    connect(req, SIGNAL(success()),
            this, SLOT(onPingServerSuccess()));
    connect(req, SIGNAL(failed(const ApiError&)),
            this, SLOT(onPingServerFailed()));
    req->setIgnoreSslErrors(true);
    req->send();
    requests_[url.host()] = req;
}


void ServerStatusService::onPingServerSuccess()
{
    PingServerRequest *req = (PingServerRequest *)sender();
    statuses_[req->url().host()] = ServerStatus(req->url(), true);
    emit serverStatusChanged();
    requests_.remove(req->url().host());
}

void ServerStatusService::onPingServerFailed()
{
    PingServerRequest *req = (PingServerRequest *)sender();
    statuses_[req->url().host()] = ServerStatus(req->url(), false);
    emit serverStatusChanged();
    requests_.remove(req->url().host());
}

bool ServerStatusService::allServersConnected() const
{
    foreach (const ServerStatus& status, statuses()) {
        if (!status.connected) {
            return false;
        }
    }

    return true;
}

bool ServerStatusService::allServersDisconnected() const
{
    foreach (const ServerStatus& status, statuses()) {
        if (status.connected) {
            return false;
        }
    }

    return true;
}
