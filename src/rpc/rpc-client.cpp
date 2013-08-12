extern "C" {
#include <ccnet.h>
#include <searpc-client.h>
}

#include <QTimer>

#include "rpc-client.h"

namespace {

const int kReconnectIntervalMilli = 2000;

} // namespace

RpcClient::RpcClient(const QString& config_dir) :
    config_dir_(config_dir)
{
    conn_daemon_timer_ = new QTimer(this);
    connect(conn_daemon_timer_, SIGNAL(timeout()), this, SLOT(connectCcnetDaemon()));
    // the time is started in RpcClient::start
}

void RpcClient::start()
{
    if (sync_client_ != 0) {
        g_object_unref (sync_client_);
    }
    sync_client_ = ccnet_client_new();

    const char *dir = config_dir_.toUtf8().data();
    if (ccnet_client_load_confdir(sync_client_, dir) <  0) {
        qCritical("failed to load ccnet config dir \"%s\"\n", dir);
    }

    conn_daemon_timer_->start(kReconnectIntervalMilli);
}

void RpcClient::connectCcnetDaemon()
{
    if (ccnet_client_connect_daemon(sync_client_, CCNET_CLIENT_SYNC) < 0) {
        return;
    }

    // Now it's connected to daemon
    conn_daemon_timer_->stop();
}
