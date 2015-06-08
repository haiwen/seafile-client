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
#include "settings-mgr.h"

#include "utils/utils.h"
#include "local-repo.h"
#include "clone-task.h"
#include "rpc-client.h"


namespace {

const char *kSeafileRpcService = "seafile-rpcserver";
const char *kCcnetRpcService = "ccnet-rpcserver";

} // namespace

#define toCStr(_s)   ((_s).isNull() ? NULL : (_s).toUtf8().data())

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

    // we are leaking memory here
    seafile_rpc_client_ = ccnet_create_rpc_client(sync_client_, NULL, kSeafileRpcService);
    ccnet_rpc_client_ = ccnet_create_rpc_client(sync_client_, NULL, kCcnetRpcService);

    qWarning("[Rpc Client] connected to daemon");
}

int SeafileRpcClient::listLocalRepos(std::vector<LocalRepo> *result)
{
    GError *error = NULL;
    GList *repos = seafile_get_repo_list(seafile_rpc_client_, 0, 0, &error);
    if (error != NULL) {
        qWarning("failed to get repo list: %s\n", error->message);
        g_error_free(error);
        return -1;
    }

    result->clear();
    for (GList *ptr = repos; ptr; ptr = ptr->next) {
        result->push_back(LocalRepo::fromGObject((GObject*)ptr->data));
    }

    g_list_foreach (repos, (GFunc)g_object_unref, NULL);
    g_list_free (repos);

    return 0;
}

int SeafileRpcClient::setAutoSync(bool autoSync)
{
    GError *error = NULL;
    int ret;
    if (autoSync) {
        ret = searpc_client_call__int (seafile_rpc_client_,
                                       "seafile_enable_auto_sync",
                                       &error, 0);
    } else {
        ret = searpc_client_call__int (seafile_rpc_client_,
                                       "seafile_disable_auto_sync",
                                       &error, 0);
    }

    if (error) {
        qWarning("failed to set auto_sync: %s\n", error->message);
        g_error_free(error);
    }

    return ret;
}

int SeafileRpcClient::downloadRepo(const QString& id,
                                   int repo_version, const QString& relayId,
                                   const QString& name, const QString& wt,
                                   const QString& token, const QString& passwd,
                                   const QString& magic, const QString& peerAddr,
                                   const QString& port, const QString& email,
                                   const QString& random_key, int enc_version,
                                   const QString& more_info,
                                   QString *error_ret)
{
    GError *error = NULL;
    char *ret = searpc_client_call__string(
        seafile_rpc_client_,
        "seafile_download",
        &error, 14,
        "string", toCStr(id),
        "int", repo_version,
        "string", toCStr(relayId),
        "string", toCStr(name),
        "string", toCStr(wt),
        "string", toCStr(token),
        "string", toCStr(passwd),
        "string", toCStr(magic),
        "string", toCStr(peerAddr),
        "string", toCStr(port),
        "string", toCStr(email),
        "string", toCStr(random_key),
        "int", enc_version,
        "string", toCStr(more_info));

    if (error != NULL) {
        if (error_ret) {
            *error_ret = error->message;
        }
        g_error_free(error);
        return -1;
    }

    g_free(ret);
    return 0;
}

int SeafileRpcClient::cloneRepo(const QString& id,
                                int repo_version, const QString& relayId,
                                const QString &name, const QString &wt,
                                const QString &token, const QString &passwd,
                                const QString &magic, const QString &peerAddr,
                                const QString &port, const QString &email,
                                const QString& random_key, int enc_version,
                                const QString& more_info,
                                QString *error_ret)
{
    GError *error = NULL;
    char *ret = searpc_client_call__string(
        seafile_rpc_client_,
        "seafile_clone",
        &error, 14,
        "string", toCStr(id),
        "int", repo_version,
        "string", toCStr(relayId),
        "string", toCStr(name),
        "string", toCStr(wt),
        "string", toCStr(token),
        "string", toCStr(passwd),
        "string", toCStr(magic),
        "string", toCStr(peerAddr),
        "string", toCStr(port),
        "string", toCStr(email),
        "string", toCStr(random_key),
        "int", enc_version,
        "string", toCStr(more_info));

    if (error != NULL) {
        if (error_ret) {
            // copy string
            *error_ret = error->message;
        }
        g_error_free(error);
        return -1;
    }

    g_free(ret);
    return 0;
}

int SeafileRpcClient::getLocalRepo(const QString& repo_id, LocalRepo *repo)
{
    GError *error = NULL;
    GObject *obj = searpc_client_call__object(
        seafile_rpc_client_,
        "seafile_get_repo",
        SEAFILE_TYPE_REPO,
        &error, 1,
        "string", toCStr(repo_id));

    if (error != NULL) {
        g_error_free(error);
        return -1;
    }

    if (obj == NULL) {
        return -1;
    }

    *repo = LocalRepo::fromGObject(obj);
    g_object_unref(obj);

    getSyncStatus(*repo);
    return 0;
}

int SeafileRpcClient::ccnetGetConfig(const QString &key, QString *value)
{
    GError *error = NULL;
    char *ret = searpc_client_call__string (ccnet_rpc_client_,
                                            "get_config", &error,
                                            1, "string", toCStr(key));
    if (error) {
        g_error_free(error);
        return -1;
    }
    *value = QString::fromUtf8(ret);

    g_free (ret);
    return 0;
}

int SeafileRpcClient::seafileGetConfig(const QString &key, QString *value)
{
    GError *error = NULL;
    char *ret = searpc_client_call__string (seafile_rpc_client_,
                                            "seafile_get_config", &error,
                                            1, "string", toCStr(key));
    if (error) {
        qWarning("Unable to get config value %s", key.toUtf8().data());
        g_error_free(error);
        return -1;
    }
    *value = QString::fromUtf8(ret);

    g_free (ret);
    return 0;
}

int SeafileRpcClient::seafileGetConfigInt(const QString &key, int *value)
{
    GError *error = NULL;
    *value = searpc_client_call__int (seafile_rpc_client_,
                                      "seafile_get_config_int", &error,
                                      1, "string", toCStr(key));
    if (error) {
        qWarning("Unable to get config (int) value %s", key.toUtf8().data());
        g_error_free(error);
        return -1;
    }
    return 0;
}

int SeafileRpcClient::ccnetSetConfig(const QString &key, const QString &value)
{
    GError *error = NULL;
    searpc_client_call__int (ccnet_rpc_client_,
                             "set_config", &error,
                             2, "string", toCStr(key),
                             "string", toCStr(value));
    if (error) {
        g_error_free(error);
        return -1;
    }
    return 0;
}

int SeafileRpcClient::seafileSetConfig(const QString &key, const QString &value)
{
    GError *error = NULL;
    searpc_client_call__int (seafile_rpc_client_,
                             "seafile_set_config", &error,
                             2, "string", toCStr(key),
                             "string", toCStr(value));
    if (error) {
        qWarning("Unable to set config value %s", key.toUtf8().data());
        g_error_free(error);
        return -1;
    }
    return 0;
}

int SeafileRpcClient::setUploadRateLimit(int limit)
{
    return setRateLimit(true, limit);
}

int SeafileRpcClient::setDownloadRateLimit(int limit)
{
    return setRateLimit(false, limit);
}

int SeafileRpcClient::setRateLimit(bool upload, int limit)
{
    GError *error = NULL;
    const char *rpc = upload ? "seafile_set_upload_rate_limit" : "seafile_set_download_rate_limit";
    searpc_client_call__int (seafile_rpc_client_,
                             rpc, &error,
                             1, "int", limit);
    if (error) {
        g_error_free(error);
        return -1;
    }
    return 0;
}

int SeafileRpcClient::seafileSetConfigInt(const QString &key, int value)
{
    GError *error = NULL;
    searpc_client_call__int (seafile_rpc_client_,
                             "seafile_set_config", &error,
                             2, "string", toCStr(key),
                             "int", value);
    if (error) {
        g_error_free(error);
        return -1;
    }
    return 0;
}

bool SeafileRpcClient::hasLocalRepo(const QString& repo_id)
{
    LocalRepo repo;
    if (getLocalRepo(repo_id, &repo) < 0) {
        return false;
    }

    return true;
}

void SeafileRpcClient::getSyncStatus(LocalRepo &repo)
{
    if (repo.worktree_invalid) {
        repo.setSyncInfo("error", "invalid worktree");
        return;
    }

    GError *error = NULL;
    SeafileSyncTask *task = (SeafileSyncTask *)
        searpc_client_call__object (seafile_rpc_client_,
                                    "seafile_get_repo_sync_task",
                                    SEAFILE_TYPE_SYNC_TASK,
                                    &error, 1,
                                    "string", toCStr(repo.id));
    if (error) {
        repo.setSyncInfo("unknown");
        g_error_free(error);
        return;
    }

    if (!task) {
        repo.setSyncInfo("waiting for sync");
        return;
    }

    char *state = NULL;
    char *err = NULL;
    g_object_get(task, "state", &state, "error", &err, NULL);

    repo.setSyncInfo(state,
                     g_strcmp0(state, "error") == 0 ? err : NULL);

    g_free (state);
    g_free (err);
    g_object_unref(task);
}

int SeafileRpcClient::getCloneTasks(std::vector<CloneTask> *tasks)
{
    GError *error = NULL;
    GList *objlist = searpc_client_call__objlist(
        seafile_rpc_client_,
        "seafile_get_clone_tasks",
        SEAFILE_TYPE_CLONE_TASK,
        &error, 0);

    if (error) {
        g_error_free(error);
        return -1;
    }

    for (GList *ptr = objlist; ptr; ptr = ptr->next) {
        CloneTask task = CloneTask::fromGObject((GObject *)ptr->data);

        if (task.state == "fetch") {
            getTransferDetail(&task);
        } else if (task.state == "checkout") {
            getCheckOutDetail(&task);
        } else if (task.state == "error") {
            if (task.error_str == "fetch") {
                getTransferDetail(&task);
            }
        }
        task.translateStateInfo();
        tasks->push_back(task);
    }

    g_list_foreach (objlist, (GFunc)g_object_unref, NULL);
    g_list_free (objlist);

    return 0;
}

void SeafileRpcClient::getTransferDetail(CloneTask* task)
{
    GError *error = NULL;
    GObject *obj = searpc_client_call__object(
        seafile_rpc_client_,
        "seafile_find_transfer_task",
        SEAFILE_TYPE_TASK,
        &error, 1,
        "string", toCStr(task->repo_id));

    if (error != NULL) {
        g_error_free(error);
        return;
    }

    if (obj == NULL) {
        return;
    }

    if (task->state == "error") {
        char *err = NULL;
        g_object_get(obj, "error_str", &err, NULL);
        task->error_str = err;
    } else {
        int block_done = 0;
        int block_total = 0;

        g_object_get (obj,
                      "block_done", &block_done,
                      "block_total", &block_total,
                      NULL);

        task->block_done = block_done;
        task->block_total = block_total;
    }

    g_object_unref (obj);
}

void SeafileRpcClient::getCheckOutDetail(CloneTask *task)
{
    GError *error = NULL;
    GObject *obj = searpc_client_call__object(
        seafile_rpc_client_,
        "seafile_get_checkout_task",
        SEAFILE_TYPE_CHECKOUT_TASK,
        &error, 1,
        "string", toCStr(task->repo_id));

    if (error != NULL) {
        g_error_free(error);
        return;
    }

    if (obj == NULL) {
        return;
    }

    int checkout_done = 0;
    int checkout_total = 0;

    g_object_get (obj,
                  "total_files", &checkout_total,
                  "finished_files", &checkout_done,
                  NULL);

    task->checkout_done = checkout_done;
    task->checkout_total = checkout_total;

    g_object_unref (obj);
}

int SeafileRpcClient::cancelCloneTask(const QString& repo_id, QString *err)
{
    GError *error = NULL;
    int ret = searpc_client_call__int (seafile_rpc_client_,
                                       "seafile_cancel_clone_task",
                                       &error, 1,
                                       "string", toCStr(repo_id));

    if (ret < 0) {
        if (err) {
            *err = error ? error->message : tr("Unknown error");
        }
        if (error) {
            g_error_free(error);
        }
    }

    return ret;
}

int SeafileRpcClient::removeCloneTask(const QString& repo_id, QString *err)
{
    GError *error = NULL;
    int ret = searpc_client_call__int (seafile_rpc_client_,
                                       "seafile_remove_clone_task",
                                       &error, 1,
                                       "string", toCStr(repo_id));

    if (ret < 0) {
        if (err) {
            *err = error ? error->message : tr("Unknown error");
        }
        if (error) {
            g_error_free(error);
        }
    }

    return ret;
}

int SeafileRpcClient::getCloneTasksCount(int *count)
{
    GError *error = NULL;
    GList *objlist = searpc_client_call__objlist(
        seafile_rpc_client_,
        "seafile_get_clone_tasks",
        SEAFILE_TYPE_CLONE_TASK,
        &error, 0);

    if (error) {
        g_error_free(error);
        return -1;
    }

    if (count) {
        *count = g_list_length(objlist);
    }

    g_list_foreach (objlist, (GFunc)g_object_unref, NULL);
    g_list_free (objlist);

    return 0;
}

int SeafileRpcClient::getServers(GList** servers)
{
    GError *error = NULL;
    GList *objlist = searpc_client_call__objlist(
        ccnet_rpc_client_,
        "get_peers_by_role",
        CCNET_TYPE_PEER,
        &error, 1,
        "string", "MyRelay");

    if (error) {
        g_error_free(error);
        return -1;
    }

    *servers = objlist;

    return 0;
}

int SeafileRpcClient::unsyncReposByAccount(const QString& server_addr,
                                           const QString& email,
                                           QString *err)
{
    GError *error = NULL;
    int ret =  searpc_client_call__int (seafile_rpc_client_,
                                        "seafile_unsync_repos_by_account",
                                        &error, 2,
                                        "string", toCStr(server_addr),
                                        "string", toCStr(email));

    if (ret < 0 && err) {
        if (error) {
            *err = QString::fromUtf8(error->message);
            g_error_free(error);
        } else {
            *err = tr("Unknown error");
        }
    }

    return ret;
}

int SeafileRpcClient::getDownloadRate(int *rate)
{
    GError *error = NULL;
    int ret = searpc_client_call__int (seafile_rpc_client_,
                                       "seafile_get_download_rate",
                                       &error, 0);

    if (error) {
        g_error_free(error);
        return -1;
    }

    *rate = ret;
    return 0;
}

int SeafileRpcClient::getUploadRate(int *rate)
{
    GError *error = NULL;
    int ret = searpc_client_call__int (seafile_rpc_client_,
                                       "seafile_get_upload_rate",
                                       &error, 0);

    if (error) {
        g_error_free(error);
        return -1;
    }

    *rate = ret;
    return 0;
}


void SeafileRpcClient::setRepoAutoSync(const QString& repo_id, bool auto_sync)
{
    GError *error = NULL;
    searpc_client_call__int(seafile_rpc_client_,
                            "seafile_set_repo_property",
                            &error, 3,
                            "string", toCStr(repo_id),
                            "string", "auto-sync",
                            "string", auto_sync ? "true" : "false");
    if (error) {
        g_error_free(error);
    }
}

int SeafileRpcClient::unsync(const QString& repo_id)
{
    GError *error = NULL;
    int ret = searpc_client_call__int(seafile_rpc_client_,
                                      "seafile_destroy_repo",
                                      &error, 1,
                                      "string", toCStr(repo_id));
    if (error) {
        g_error_free(error);
    }

    return ret;
}

int SeafileRpcClient::getRepoTransferInfo(const QString& repo_id, int *rate, int *percent)
{
    GError *error = NULL;
    GObject *task = searpc_client_call__object (seafile_rpc_client_,
                                                "seafile_find_transfer_task",
                                                SEAFILE_TYPE_TASK,
                                                &error, 1,
                                                "string", toCStr(repo_id));
    if (error) {
        g_error_free(error);
        return -1;
    }

    if (!task) {
        return -1;
    }

    int finished = 0;
    int total = 0;
    g_object_get (task,
                  "rate", rate,
                  "block_total", &total,
                  "block_done", &finished,
                  NULL);

    if (total == 0) {
        *percent = 0;
    } else {
        *percent = 100 * finished / total;
    }

    g_object_unref(task);

    return 0;
}

void SeafileRpcClient::syncRepoImmediately(const QString& repo_id)
{
    searpc_client_call__int (seafile_rpc_client_,
                             "seafile_sync", NULL,
                             2,
                             "string", toCStr(repo_id),
                             "string", NULL);
}

int SeafileRpcClient::checkPathForClone(const QString& path, QString *err_msg)
{
    GError *error = NULL;
    int ret = searpc_client_call__int (seafile_rpc_client_,
                                       "seafile_check_path_for_clone", &error,
                                       1, "string", toCStr(path));

    if (ret == 0) {
        return 0;
    }

    QString err;
    const char *msg = error->message;
    if (g_strcmp0(msg, "Worktree conflicts system path") == 0) {
        err = tr("The path \"%1\" conflicts with system path").arg(path);
    } else if (g_strcmp0(msg, "Worktree conflicts existing repo") == 0) {
        err = tr("The path \"%1\" conflicts with an existing library").arg(path);
    } else {
        err = QString::fromUtf8(msg);
    }
    g_error_free(error);

    if (err_msg) {
        *err_msg = err;
    }

    return -1;
}

QString SeafileRpcClient::getCcnetPeerId()
{
    return sync_client_ ? sync_client_->base.id : "";
}

int SeafileRpcClient::updateReposServerHost(const QString& old_host,
                                            const QString& new_host,
                                            const QString& new_server_url,
                                            QString *err)
{
    GError *error = NULL;
    int ret =  searpc_client_call__int (seafile_rpc_client_,
                                        "seafile_update_repos_server_host",
                                        &error, 3,
                                        "string", toCStr(old_host),
                                        "string", toCStr(new_host),
                                        "string", toCStr(new_server_url));

    if (ret < 0) {
        if (error) {
            *err = QString::fromUtf8(error->message);
            g_error_free(error);
        } else {
            *err = tr("Unknown error");
        }
    }

    return ret;
}

int SeafileRpcClient::getRepoProperty(const QString &repo_id,
                                      const QString& name,
                                      QString *value)
{
    GError *error = NULL;
    char *ret = searpc_client_call__string (
        seafile_rpc_client_,
        "seafile_get_repo_property",
        &error, 2,
        "string", toCStr(repo_id),
        "string", toCStr(name)
        );
    if (error) {
        g_error_free(error);
        return -1;
    }
    *value = QString::fromUtf8(ret);

    g_free(ret);
    return 0;
}

int SeafileRpcClient::setRepoProperty(const QString &repo_id,
                                      const QString& name,
                                      const QString& value)
{
    GError *error = NULL;
    int ret = searpc_client_call__int (
        seafile_rpc_client_,
        "seafile_set_repo_property",
        &error, 3,
        "string", toCStr(repo_id),
        "string", toCStr(name),
        "string", toCStr(value)
        );
    if (error) {
        g_error_free(error);
        return -1;
    }
    return ret;
}

int SeafileRpcClient::removeSyncTokensByAccount(const QString& server_addr,
                                                const QString& email,
                                                QString *err)
{
    GError *error = NULL;
    int ret =  searpc_client_call__int (seafile_rpc_client_,
                                        "seafile_remove_repo_tokens_by_account",
                                        &error, 2,
                                        "string", toCStr(server_addr),
                                        "string", toCStr(email));

    if (ret < 0 && err) {
        if (error) {
            *err = QString::fromUtf8(error->message);
            g_error_free(error);
        } else {
            *err = tr("Unknown error");
        }
    }

    return ret;
}

int SeafileRpcClient::setRepoToken(const QString &repo_id,
                                   const QString& token)
{
    GError *error = NULL;
    int ret = searpc_client_call__int (
        seafile_rpc_client_,
        "seafile_set_repo_token",
        &error, 2,
        "string", toCStr(repo_id),
        "string", toCStr(token)
        );
    if (error) {
        g_error_free(error);
        return -1;
    }
    return ret;
}

int SeafileRpcClient::getRepoFileStatus(const QString& repo_id,
                                        const QString& path_in_repo,
                                        bool isdir,
                                        QString *status)
{
    GError *error = NULL;
    char *ret = searpc_client_call__string (
        seafile_rpc_client_,
        "seafile_get_path_sync_status",
        &error, 3,
        "string", toCStr(repo_id),
        "string", toCStr(path_in_repo),
        "int", isdir ? 1 : 0);
    if (error) {
        qWarning("failed to get path status: %s\n", error->message);
        g_error_free(error);
        return -1;
    }

    *status = ret;

    g_free (ret);
    return 0;
}
