extern "C" {
#include <searpc-client.h>
#include <ccnet.h>

#include <searpc.h>
#include <seafile/seafile.h>
#include <seafile/seafile-object.h>

}

#include <QSocketNotifier>

#include "seafile-applet.h"
#include "configurator.h"

#include "local-repo.h"
#include "rpc-client.h"


namespace {

const char *kSeafileRpcService = "seafile-rpcserver";
const char *kCcnetRpcService = "ccnet-rpcserver";

} // namespace


SeafileRpcClient::SeafileRpcClient()
      : async_client_(0),
        seafile_rpc_client_(0),
        ccnet_rpc_client_(0),
        socket_notifier_(0)
{
}

void SeafileRpcClient::connectDaemon()
{
    async_client_ = ccnet_client_new();

    const QString config_dir = seafApplet->configurator()->ccnetDir();
    const QByteArray path = config_dir.toUtf8();
    if (ccnet_client_load_confdir(async_client_, path.data()) <  0) {
        seafApplet->errorAndExit(tr("failed to load ccnet config dir %1").arg(config_dir));
    }

    if (ccnet_client_connect_daemon(async_client_, CCNET_CLIENT_ASYNC) < 0) {
        return;
    }

    seafile_rpc_client_ = ccnet_create_async_rpc_client(async_client_, NULL, kSeafileRpcService);
    ccnet_rpc_client_ = ccnet_create_async_rpc_client(async_client_, NULL, kCcnetRpcService);

    socket_notifier_ = new QSocketNotifier(async_client_->connfd, QSocketNotifier::Read);
    connect(socket_notifier_, SIGNAL(activated(int)), this, SLOT(readConnfd()));

    qDebug("[Rpc Client] connected to daemon");
}

void SeafileRpcClient::readConnfd()
{
    socket_notifier_->setEnabled(false);
    if (ccnet_client_read_input(async_client_) <= 0) {
        return;
    }
    socket_notifier_->setEnabled(true);
}

void SeafileRpcClient::listLocalRepos()
{
    searpc_client_async_call__objlist (
        seafile_rpc_client_,
        "seafile_get_repo_list", (AsyncCallback)listLocalReposCB,
        SEAFILE_TYPE_REPO, this, 0);
}

void SeafileRpcClient::setAutoSync(bool autoSync)
{
    if (autoSync)
        seafile_enable_auto_sync_async (seafile_rpc_client_,
                                        (AsyncCallback)setAutoSyncCB,
                                        this);
    else
        seafile_disable_auto_sync_async (seafile_rpc_client_,
                                         (AsyncCallback)setNotAutoSyncCB,
                                         this);
}

void SeafileRpcClient::listLocalReposCB(void *result, void *data, GError *error)
{
    SeafileRpcClient *client = static_cast<SeafileRpcClient*>(data);
    std::vector<LocalRepo> repos;

    if (error == NULL) {
        qDebug("list repos success");
        GList *obj_list = (GList *)result;
        for (GList *ptr = obj_list; ptr; ptr = ptr->next) {
            GObject *obj = static_cast<GObject*>(ptr->data);
            repos.push_back(LocalRepo::fromGObject((GObject*)obj));
        }

        emit client->listLocalReposSignal(repos, true);
    } else {
        qDebug("list repos failed %s", error->message);
        emit client->listLocalReposSignal(repos, false);
    }
}

void SeafileRpcClient::setAutoSyncCB(void *result, void *data, GError *error)
{
    SeafileRpcClient *client = static_cast<SeafileRpcClient*>(data);

    if (error)
        emit client->setAutoSyncSignal(true, false);
    else
        emit client->setAutoSyncSignal(true, true);
}

void SeafileRpcClient::setNotAutoSyncCB(void *result, void *data, GError *error)
{
    SeafileRpcClient *client = static_cast<SeafileRpcClient*>(data);

    if (error)
        emit client->setAutoSyncSignal(false, false);
    else
        emit client->setAutoSyncSignal(false, true);
}
