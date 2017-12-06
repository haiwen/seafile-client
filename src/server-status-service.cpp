#include <QTimer>

#include "server-status-service.h"
#include "seafile-applet.h"
#include "account-mgr.h"
#include "api/api-error.h"
#include "api/requests.h"

namespace {

const int kRefreshInterval = 3 * 60 * 1000; // 3 min
const int kRefreshIntervalForUnconnected = 30 * 1000; // 30 sec

}

SINGLETON_IMPL(ServerStatusService)

ServerStatusService::ServerStatusService(QObject *parent)
    : QObject(parent)
{
    refresh_timer_ = new QTimer(this);
    refresh_unconnected_timer_ = new QTimer(this);
    connect(refresh_timer_, SIGNAL(timeout()),
            this, SLOT(refresh()));
    connect(refresh_unconnected_timer_, SIGNAL(timeout()),
            this, SLOT(refreshUnconnected()));
    refresh();
}

void ServerStatusService::start()
{
    refresh_timer_->start(kRefreshInterval);
    refresh_unconnected_timer_->start(kRefreshIntervalForUnconnected);
}

void ServerStatusService::stop()
{
    refresh_timer_->stop();
    refresh_unconnected_timer_->stop();
}

void ServerStatusService::refresh(bool only_refresh_unconnected)
{
    const std::vector<Account>& accounts = seafApplet->accountManager()->accounts();
    for (size_t i = 0; i < accounts.size(); i++) {
        const QUrl& url = accounts[i].serverUrl;
        if (requests_.contains(url.host())) {
            continue;
        }

        if (!statuses_.contains(url.host())) {
            statuses_[url.host()] = ServerStatus(url, true);
        }

        if (only_refresh_unconnected && isServerConnected(url)) {
            continue;
        }
        pingServer(url);
    }
}

void ServerStatusService::pingServer(const QUrl& url)
{
    PingServerRequest *req = new PingServerRequest(url);
    connect(req, SIGNAL(success()),
            this, SLOT(onPingServerSuccess()));
    connect(req, SIGNAL(failed(const ApiError&)),
            this, SLOT(onPingServerFailed()));
    req->send();
    requests_[url.host()] = req;
}


void ServerStatusService::onPingServerSuccess()
{
    PingServerRequest *req = (PingServerRequest *)sender();
    statuses_[req->url().host()] = ServerStatus(req->url(), true);
    emit serverStatusChanged();
    requests_.take(req->url().host())->deleteLater();
}

void ServerStatusService::onPingServerFailed()
{
    PingServerRequest *req = (PingServerRequest *)sender();
    statuses_[req->url().host()] = ServerStatus(req->url(), false);
    emit serverStatusChanged();
    requests_.take(req->url().host())->deleteLater();
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


bool ServerStatusService::isServerConnected(const QUrl& url) const
{
    return statuses_.value(url.host()).connected;
}

void ServerStatusService::updateOnSuccessfullRequest(const QUrl& url)
{
    updateOnRequestFinished(url, true);
}

void ServerStatusService::updateOnFailedRequest(const QUrl& url)
{
    updateOnRequestFinished(url, false);
}

void ServerStatusService::updateOnRequestFinished(const QUrl& url, bool no_network_error)
{
    bool found = false;
    const std::vector<Account>& accounts = seafApplet->accountManager()->accounts();
    for (size_t i = 0; i < accounts.size(); i++) {
        if (url.host() == accounts[i].serverUrl.host()) {
            found = true;
            break;
        }
    }
    if (!found) {
        qWarning("ServerStatusService: ignore request for host \"%s\"", url.host().toUtf8().data());
        return;
    }

    bool changed = false;
    if (statuses_.contains(url.host())) {
        const ServerStatus& status = statuses_[url.host()];
        if (status.connected != no_network_error) {
            changed = true;
        }
    }
    statuses_[url.host()] = ServerStatus(url, no_network_error);

    if (changed) {
        emit serverStatusChanged();
    }
}
