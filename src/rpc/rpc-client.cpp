extern "C" {
#include <searpc-client.h>
#include <ccnet.h>
#include <seafile/seafile.h>
}

#include <QTimer>
#include <QDir>

#include "rpc-client.h"

namespace {

const int kReconnectIntervalMilli = 2000;
const char *kSeafileRpcService = "seafile-rpcserver";

} // namespace

RpcClient::RpcClient(const QString& config_dir)
    : config_dir_(config_dir),
      sync_client_(0),
      seafile_rpc_client_(0),
      ccnet_rpc_client_(0),
      conn_daemon_timer_(0)
{
    qDebug("config dir is %s\n", config_dir.toUtf8().data());
    // the time is started in RpcClient::start
    conn_daemon_timer_ = new QTimer(this);
    connect(conn_daemon_timer_, SIGNAL(timeout()), this, SLOT(connectCcnetDaemon()));
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

bool RpcClient::connected()
{
    return sync_client_ != 0 && sync_client_->connected;
}

void RpcClient::connectCcnetDaemon()
{
    if (ccnet_client_connect_daemon(sync_client_, CCNET_CLIENT_SYNC) < 0) {
        return;
    }

    if (seafile_rpc_client_ != 0) {
        searpc_client_free(seafile_rpc_client_);
    }

    seafile_rpc_client_ = ccnet_create_rpc_client(sync_client_, NULL, kSeafileRpcService);

    // Now it's connected to daemon
    conn_daemon_timer_->stop();
}

int RpcClient::listRepos(std::vector<LocalRepo> *result)
{
    GError *error = NULL;
    GList *repos = seafile_get_repo_list(seafile_rpc_client_, 0, 0, &error);
    if (repos == NULL) {
        qWarning("failed to get repo list: %s\n", error->message);
        return -1;
    }

    for (GList *ptr = repos; ptr; ptr = ptr->next) {
        GObject *obj = static_cast<GObject*>(ptr->data);
        result->push_back(LocalRepo::fromGObject((GObject*)obj));
    }

    g_list_foreach (repos, (GFunc)g_object_unref, NULL);

    g_list_free (repos);

    return 0;
}
