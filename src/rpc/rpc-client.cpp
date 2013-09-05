extern "C" {
#include <searpc-client.h>
#include <ccnet.h>

#include <searpc.h>
#include <seafile/seafile.h>
#include <seafile/seafile-object.h>

}

#include <QtDebug>
#include "seafile-applet.h"
#include "configurator.h"

#include "utils/utils.h"
#include "local-repo.h"
#include "rpc-client.h"


namespace {

const char *kSeafileRpcService = "seafile-rpcserver";
const char *kCcnetRpcService = "ccnet-rpcserver";

} // namespace

SeafileRpcClient::SeafileRpcClient()
      : sync_client_(0),
        seafile_rpc_client_(0),
        ccnet_rpc_client_(0)
{
}

void SeafileRpcClient::connectDaemon()
{
    sync_client_ = ccnet_client_new();

    const QString config_dir = seafApplet->configurator()->ccnetDir();
    if (ccnet_client_load_confdir(sync_client_, toCStr(config_dir)) <  0) {
        seafApplet->errorAndExit(tr("failed to load ccnet config dir %1").arg(config_dir));
    }

    if (ccnet_client_connect_daemon(sync_client_, CCNET_CLIENT_SYNC) < 0) {
        return;
    }

    seafile_rpc_client_ = ccnet_create_rpc_client(sync_client_, NULL, kSeafileRpcService);
    ccnet_rpc_client_ = ccnet_create_rpc_client(sync_client_, NULL, kCcnetRpcService);

    qDebug("[Rpc Client] connected to daemon");
}

int SeafileRpcClient::listLocalRepos(std::vector<LocalRepo> *result)
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

int SeafileRpcClient::setAutoSync(bool autoSync)
{
    GError *error = NULL;
    if (autoSync) {
        int ret = searpc_client_call__int (seafile_rpc_client_,
                                           "seafile_enable_auto_sync",
                                           &error, 0);
        return ret;
    } else {
        int ret = searpc_client_call__int (seafile_rpc_client_,
                                           "seafile_disable_auto_sync",
                                           &error, 0);
        return ret;
    }

    return 0;
}

int SeafileRpcClient::downloadRepo(const QString &id, const QString &relayId,
                                    const QString &name, const QString &wt,
                                    const QString &token, const QString &passwd,
                                    const QString &magic, const QString &peerAddr,
                                    const QString &port, const QString &email)
{
    GError *error = NULL;
    searpc_client_call__string(
        seafile_rpc_client_,
        "seafile_download",
        &error, 10,
        "string", toCStr(id),
        "string", toCStr(relayId),
        "string", toCStr(name),
        "string", toCStr(wt),
        "string", toCStr(token),
        "string", toCStr(passwd),
        "string", toCStr(magic),
        "string", toCStr(peerAddr),
        "string", toCStr(port),
        "string", toCStr(email));

    if (error != NULL) {
        return -1;
    }

    return 0;
}

int SeafileRpcClient::cloneRepo(const QString &id, const QString &relayId,
                                 const QString &name, const QString &wt,
                                 const QString &token, const QString &passwd,
                                 const QString &magic, const QString &peerAddr,
                                 const QString &port, const QString &email)
{
    GError *error = NULL;
    searpc_client_call__string (
        seafile_rpc_client_,
        "seafile_clone",
        &error, 10,
        "string", toCStr(id),
        "string", toCStr(relayId),
        "string", toCStr(name),
        "string", toCStr(wt),
        "string", toCStr(token),
        "string", toCStr(passwd),
        "string", toCStr(magic),
        "string", toCStr(peerAddr),
        "string", toCStr(port),
        "string", toCStr(email));

    if (error != NULL) {
        return -1;
    }

    return 0;
}
