extern "C" {
#include <searpc-client.h>
#include <ccnet.h>

#include <searpc.h>
#include <seafile/seafile.h>
#include <seafile/seafile-object.h>

}

#include <QSocketNotifier>
#include <QtDebug>
#include "seafile-applet.h"
#include "configurator.h"

#include "local-repo.h"
#include "rpc-client.h"


namespace {

    const char *kSeafileRpcService = "seafile-rpcserver";
    const char *kCcnetRpcService = "ccnet-rpcserver";

} // namespace

#define toCStr(_s)   (_s.isNull() ? NULL : _s.toUtf8().data())

struct DownloadRepoData {
    SeafileRpcClient *client;
    QString repoId;
    DownloadRepoData(SeafileRpcClient *cli, const QString &id):client(cli), repoId(id){};
};

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
    if (ccnet_client_load_confdir(async_client_, toCStr(config_dir)) <  0) {
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

void SeafileRpcClient::downloadRepo(const QString &id, const QString &relayId,
                                    const QString &name, const QString &wt,
                                    const QString &token, const QString &passwd,
                                    const QString &magic, const QString &peerAddr,
                                    const QString &port, const QString &email)
{
    DownloadRepoData *data = new DownloadRepoData(this, id);
    searpc_client_async_call__string (
        seafile_rpc_client_,
        "seafile_download", (AsyncCallback)downloadRepoCB,
        data, 10, "string", toCStr(id),
        "string", toCStr(relayId),
        "string", toCStr(name),
        "string", toCStr(wt),
        "string", toCStr(token),
        "string", toCStr(passwd),
        "string", toCStr(magic),
        "string", toCStr(peerAddr),
        "string", toCStr(port),
        "string", toCStr(email));
}

void SeafileRpcClient::cloneRepo(const QString &id, const QString &relayId,
                                 const QString &name, const QString &wt,
                                 const QString &token, const QString &passwd,
                                 const QString &magic, const QString &peerAddr,
                                 const QString &port, const QString &email)
{
    DownloadRepoData *data = new DownloadRepoData(this, id);
    searpc_client_async_call__string (
        seafile_rpc_client_,
        "seafile_clone", (AsyncCallback)cloneRepoCB,
        data, 10, "string", toCStr(id),
        "string", toCStr(relayId),
        "string", toCStr(name),
        "string", toCStr(wt),
        "string", toCStr(token),
        "string", toCStr(passwd),
        "string", toCStr(magic),
        "string", toCStr(peerAddr),
        "string", toCStr(port),
        "string", toCStr(email));
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

void SeafileRpcClient::downloadRepoCB(void *result, void *data, GError *error)
{
    DownloadRepoData *repoData = (DownloadRepoData *)data;
    SeafileRpcClient *client = static_cast<SeafileRpcClient*>(repoData->client);

    if (error) {
        qDebug() << __FUNCTION__ << " error:" << error->message;
        emit client->downloadRepoSignal(repoData->repoId, false);
    } else
        emit client->downloadRepoSignal(repoData->repoId, true);

    delete repoData;
}

void SeafileRpcClient::cloneRepoCB(void *result, void *data, GError *error)
{
    DownloadRepoData *repoData = (DownloadRepoData *)data;
    SeafileRpcClient *client = static_cast<SeafileRpcClient*>(repoData->client);

    if (error) {
        qDebug() << __FUNCTION__ << " error:" << error->message;
        emit client->cloneRepoSignal(repoData->repoId, false);
    } else
        emit client->cloneRepoSignal(repoData->repoId, true);

    delete repoData;
}
