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
#include "rpc-client.h"
#include "local-repo.h"
#include "rpc-request.h"

#define toCStr(_s)   ((_s).isNull() ? NULL : (_s).toUtf8().data())


SeafileRpcRequest::SeafileRpcRequest() {
}

/**
 * When a request is finished, it deletes itself automatically. So the object
 * who intialize a request doesn't need to call QObjec::disconnect() to
 * disconnect the signals
 */
void SeafileRpcRequest::cleanup()
{
    deleteLater();
}

SearpcClient* SeafileRpcRequest::getSeafRpcClient()
{
    return seafApplet->rpcClient()->seafRpcClient();
}

SearpcClient* SeafileRpcRequest::getCcnetRpcClient()
{
    return seafApplet->rpcClient()->ccnetRpcClient();
}

void SeafileRpcRequest::listLocalRepos()
{
    searpc_client_async_call__objlist (
        getSeafRpcClient(),
        "seafile_get_repo_list", (AsyncCallback)listLocalReposCB,
        SEAFILE_TYPE_REPO, this, 0);
}

void SeafileRpcRequest::setAutoSync(bool autoSync)
{
    if (autoSync)
        seafile_enable_auto_sync_async (getSeafRpcClient(),
                                        (AsyncCallback)setAutoSyncCB,
                                        this);
    else
        seafile_disable_auto_sync_async (getSeafRpcClient(),
                                         (AsyncCallback)setNotAutoSyncCB,
                                         this);
}

void SeafileRpcRequest::downloadRepo(const QString &id, const QString &relayId,
                                     const QString &name, const QString &wt,
                                     const QString &token, const QString &passwd,
                                     const QString &magic, const QString &peerAddr,
                                     const QString &port, const QString &email)
{
    searpc_client_async_call__string (
        getSeafRpcClient(),
        "seafile_download", (AsyncCallback)downloadRepoCB,
        this, 10, "string", toCStr(id),
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

void SeafileRpcRequest::cloneRepo(const QString &id, const QString &relayId,
                                  const QString &name, const QString &wt,
                                  const QString &token, const QString &passwd,
                                  const QString &magic, const QString &peerAddr,
                                  const QString &port, const QString &email)
{
    searpc_client_async_call__string (
        getSeafRpcClient(),
        "seafile_clone", (AsyncCallback)cloneRepoCB,
        this, 10, "string", toCStr(id),
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

void SeafileRpcRequest::listLocalReposCB(void *result, void *data, GError *error)
{
    SeafileRpcRequest *request = static_cast<SeafileRpcRequest*>(data);
    std::vector<LocalRepo> repos;

    if (error == NULL) {
        qDebug("list repos success");
        GList *obj_list = (GList *)result;
        for (GList *ptr = obj_list; ptr; ptr = ptr->next) {
            GObject *obj = static_cast<GObject*>(ptr->data);
            repos.push_back(LocalRepo::fromGObject((GObject*)obj));
        }

        emit request->listLocalReposSignal(repos, true);
    } else {
        qDebug("list repos failed %s", error->message);
        emit request->listLocalReposSignal(repos, false);
    }

    request->cleanup();
}

void SeafileRpcRequest::setAutoSyncCB(void *result, void *data, GError *error)
{
    SeafileRpcRequest *request = static_cast<SeafileRpcRequest*>(data);

    if (error)
        emit request->setAutoSyncSignal(true, false);
    else
        emit request->setAutoSyncSignal(true, true);

    request->cleanup();
}

void SeafileRpcRequest::setNotAutoSyncCB(void *result, void *data, GError *error)
{
    SeafileRpcRequest *request = static_cast<SeafileRpcRequest*>(data);

    if (error)
        emit request->setAutoSyncSignal(false, false);
    else
        emit request->setAutoSyncSignal(false, true);

    request->cleanup();
}

void SeafileRpcRequest::downloadRepoCB(void *result, void *data, GError *error)
{
    SeafileRpcRequest *request = static_cast<SeafileRpcRequest*>(data);

    if (error) {
        qDebug() << __FUNCTION__ << " error:" << error->message;
        emit request->downloadRepoSignal(request->downloadRepoID, false);
    } else
        emit request->downloadRepoSignal(request->downloadRepoID, true);

    request->cleanup();
}

void SeafileRpcRequest::cloneRepoCB(void *result, void *data, GError *error)
{
    SeafileRpcRequest *request = static_cast<SeafileRpcRequest*>(data);

    if (error) {
        qDebug() << __FUNCTION__ << " error:" << error->message;
        emit request->cloneRepoSignal(request->downloadRepoID, false);
    } else
        emit request->cloneRepoSignal(request->downloadRepoID, true);

    request->cleanup();
}
