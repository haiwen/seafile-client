extern "C" {

#include <searpc-client.h>
#include <searpc-named-pipe-transport.h>

#include <searpc.h>
#include <seafile/seafile.h>
#include <seafile/seafile-object.h>

}

#include <QtDebug>
#include <QMutexLocker>

#include "seafile-applet.h"
#include "configurator.h"
#include "settings-mgr.h"

#include "utils/utils.h"
#include "local-repo.h"
#include "clone-task.h"
#include "sync-error.h"
#include "api/commit-details.h"

#if defined(Q_OS_WIN32)
  #include "utils/utils-win.h"
#endif

#include "rpc-client.h"


namespace {

#if defined(Q_OS_WIN32)
const char *kSeafileSockName = "\\\\.\\pipe\\seafile_";
#else
const char *kSeafileSockName = "seafile.sock";
#endif
const char *kSeafileRpcService = "seafile-rpcserver";
const char *kSeafileThreadedRpcService = "seafile-threaded-rpcserver";

QString getSeafileRpcPipePath()
{
#if defined(Q_OS_WIN32)
    return utils::win::getLocalPipeName(kSeafileSockName).c_str();
#else
    return QDir(seafApplet->configurator()->seafileDir()).filePath(kSeafileSockName);
#endif
}

SearpcClient *createSearpcClientWithPipeTransport(const char *rpc_service)
{
    SearpcNamedPipeClient *pipe_client;
    pipe_client = searpc_create_named_pipe_client(toCStr(getSeafileRpcPipePath()));
    int ret = searpc_named_pipe_client_connect(pipe_client);
    SearpcClient *c = searpc_client_with_named_pipe_transport(pipe_client, rpc_service);
    if (ret < 0) {
        searpc_free_client_with_pipe_transport(c);
        return nullptr;
    }
    return c;
}

} // namespace

SeafileRpcClient::SeafileRpcClient()
    : connected_(false),
      seafile_rpc_client_(nullptr),
      seafile_threaded_rpc_client_(nullptr)
{
}

SeafileRpcClient::~SeafileRpcClient()
{
    if (seafile_rpc_client_) {
        searpc_free_client_with_pipe_transport(seafile_rpc_client_);
        seafile_rpc_client_ = nullptr;
    }
    if (seafile_threaded_rpc_client_) {
        searpc_free_client_with_pipe_transport(seafile_threaded_rpc_client_);
        seafile_threaded_rpc_client_ = nullptr;
    }
}

bool SeafileRpcClient::connectDaemon(bool exit_on_error)
{
    int retry = 0;
    while (true) {
        seafile_rpc_client_ = createSearpcClientWithPipeTransport(kSeafileRpcService);
        if (!seafile_rpc_client_) {
            if (retry++ > 20) {
                if (exit_on_error) {
                    seafApplet->errorAndExit(tr("internal error: failed to connect to seafile daemon"));
                }
                return false;
            } else {
                g_usleep(500000);
            }
        } else {
            // Create the searpc client for threaded rpc calls
            seafile_threaded_rpc_client_ = createSearpcClientWithPipeTransport(kSeafileThreadedRpcService);
            if (!seafile_threaded_rpc_client_) {
                searpc_free_client_with_pipe_transport(seafile_rpc_client_);
                seafile_rpc_client_ = nullptr;
                continue;
            }
            break;
        }
    }

    connected_ = true;
    qWarning("[Rpc Client] connected to daemon");
    return true;
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

int SeafileRpcClient::seafileGetConfig(const QString &key,
                                       QString *value)
{
    GError *error = NULL;
    char *ret = searpc_client_call__string (seafile_rpc_client_,
                                            "seafile_get_config", &error,
                                            1, "string", toCStr(key));
    if (error) {
        g_error_free(error);
        return -1;
    }
    *value = QString::fromUtf8(ret);

    g_free (ret);
    return 0;
}

int SeafileRpcClient::seafileGetConfigInt(const QString &key,
                                          int *value)
{
    GError *error = NULL;
    *value = searpc_client_call__int (seafile_rpc_client_,
                                      "seafile_get_config_int", &error,
                                      1, "string", toCStr(key));
    if (error) {
        g_error_free(error);
        return -1;
    }
    return 0;
}

int SeafileRpcClient::seafileSetConfig(const QString &key, const QString &value)
{
    // printf ("set config: %s = %s\n", toCStr(key), toCStr(value));
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
    return setRateLimit(UPLOAD, limit);
}

int SeafileRpcClient::setDownloadRateLimit(int limit)
{
    return setRateLimit(DOWNLOAD, limit);
}

int SeafileRpcClient::setRateLimit(Direction direction, int limit)
{
    GError *error = NULL;
    const char *rpc = direction == UPLOAD ? "seafile_set_upload_rate_limit"
                                          : "seafile_set_download_rate_limit";
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
    // printf ("set config: %s = %d\n", toCStr(key), value);
    GError *error = NULL;
    searpc_client_call__int (seafile_rpc_client_,
                             "seafile_set_config_int", &error,
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
    char *err_detail = NULL;
    g_object_get(task, "state", &state, "error", &err, "err_detail", &err_detail, NULL);

    // seaf-daemon would retry three times for errors like quota/permission
    // before setting the "state" field to "error", but the GUI should display
    // the error from the beginning.
    if (err != NULL && strlen(err) > 0 && strcmp(err, "Success") != 0) {
        state = g_strdup("error");
    }

    repo.setSyncInfo(state,
                     g_strcmp0(state, "error") == 0 ? err : NULL,
                     err_detail);

    if (repo.sync_state == LocalRepo::SYNC_STATE_ING) {
        getRepoTransferInfo(repo.id, &repo.transfer_rate, &repo.transfer_percentage, &repo.rt_state);
    }

    // When uploading fs objects, we show it as "uploading files list" and don't
    // show the current precentage
    if (QString(state) == "uploading" && repo.rt_state == "fs") {
        repo.sync_state_str = QObject::tr("uploading file list");
        repo.has_data_transfer = false;
    }

    g_free (state);
    g_free (err);
    g_free (err_detail);
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
        } else if (task.state == "error") {
            if (!task.error_detail.isNull())
                task.error_str = task.error_detail;
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

    if (task->state == "fetch") {
        char *rt_state = NULL;
        g_object_get (obj, "rt_state", &rt_state, NULL);
        task->rt_state = rt_state;
        g_free (rt_state);

        if (task->rt_state == "data") {
            qint64 block_done = 0;
            qint64 block_total = 0;

            g_object_get (obj,
                          "block_done", &block_done,
                          "block_total", &block_total,
                          NULL);

            task->block_done = block_done;
            task->block_total = block_total;
        } else if (task->rt_state == "fs") {
            int fs_objects_done = 0;
            int fs_objects_total = 0;

            g_object_get (obj,
                          "fs_objects_done", &fs_objects_done,
                          "fs_objects_total", &fs_objects_total,
                          NULL);

            task->fs_objects_done = fs_objects_done;
            task->fs_objects_total = fs_objects_total;
        }
    }

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

int SeafileRpcClient::getRepoTransferInfo(const QString& repo_id, int *rate, int *percent, QString *rt_state)
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

    int64_t finished = 0;
    int64_t total = 0;
    char *rt = nullptr;
    g_object_get (task,
                  "rate", rate,
                  "block_total", &total,
                  "block_done", &finished,
                  "rt_state", &rt,
                  NULL);

    if (total == 0) {
        *percent = 0;
    } else {
        *percent = (int)(100 * finished / total);
    }

    if (rt) {
        if (rt_state) {
            *rt_state = rt;
        }
        g_free(rt);
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

    if (err_msg) {
        *err_msg = QString::fromUtf8(error->message);
    }

    g_error_free(error);
    return -1;
}

QString SeafileRpcClient::getCcnetPeerId()
{
    // TODO: Get the device id now that ccnet is removed.
    return "";
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

int SeafileRpcClient::markFileLockState(const QString &repo_id,
                                        const QString &path_in_repo,
                                        bool lock)
{
    GError *error = NULL;
    char *ret = searpc_client_call__string (
        seafile_rpc_client_,
        lock ? "seafile_mark_file_locked" : "seafile_mark_file_unlocked",
        &error, 2,
        "string", toCStr(repo_id),
        "string", toCStr(path_in_repo));
    if (error) {
        qWarning("failed to mark file lock state: %s\n", error->message);
        g_error_free(error);
        return -1;
    }

    g_free (ret);
    return 0;
}

int SeafileRpcClient::generateMagicAndRandomKey(int enc_version,
                                                const QString &repo_id,
                                                const QString &passwd,
                                                QString *magic,
                                                QString *random_key)
{
    GError *error = NULL;
    GObject *obj = searpc_client_call__object (
        seafile_rpc_client_,
        "seafile_generate_magic_and_random_key",
        SEAFILE_TYPE_ENCRYPTION_INFO,
        &error, 3,
        "int", enc_version,
        "string", toCStr(repo_id),
        "string", toCStr(passwd));
    if (error) {
        qWarning("failed to generate magic and random_key : %s\n", error->message);
        g_error_free(error);
        return -1;
    }

    char *c_magic = NULL;
    char *c_random_key = NULL;
    g_object_get (obj,
                  "magic", &c_magic,
                  "random_key", &c_random_key,
                  NULL);

    *magic = QString(c_magic);
    *random_key = QString(c_random_key);

    g_object_unref (obj);
    g_free (c_magic);
    g_free (c_random_key);
    return 0;
}

bool SeafileRpcClient::setServerProperty(const QString &url,
                                         const QString &key,
                                         const QString &value)
{
    // printf("set server config: %s %s = %s\n", toCStr(url), toCStr(key),
    //        toCStr(value));
    GError *error = NULL;
    searpc_client_call__int(seafile_rpc_client_, "seafile_set_server_property",
                            &error, 3, "string", toCStr(url), "string",
                            toCStr(key), "string", toCStr(value));
    if (error) {
        qWarning("Unable to set server property %s %s", toCStr(url),
                 toCStr(key));
        g_error_free(error);
        return false;
    }
    return true;
}

bool SeafileRpcClient::getCommitDiff(const QString& repo_id,
                                     const QString& commit_id,
                                     const QString& previous_commit_id,
                                     CommitDetails *details)
{
    QMutexLocker locker(&threaded_rpc_mutex_);

    GError *error = NULL;
    GList *objlist = searpc_client_call__objlist(
        seafile_threaded_rpc_client_,
        "seafile_diff",
        SEAFILE_TYPE_DIFF_ENTRY,
        &error, 4,
        "string", toCStr(repo_id),
        "string", toCStr(commit_id),
        "string", toCStr(previous_commit_id),
        "int", 1);

    if (error) {
        qWarning("failed to get changes in commit %.7s of repo %.7s", toCStr(commit_id), toCStr(repo_id));
        g_error_free(error);
        return false;
    }

    *details = CommitDetails::fromObjList(objlist);

    g_list_foreach (objlist, (GFunc)g_object_unref, NULL);
    g_list_free (objlist);
    return true;
}

bool SeafileRpcClient::getSyncErrors(std::vector<SyncError> *errors, int offset, int limit)
{
    GError *error = NULL;
    GList *objlist = searpc_client_call__objlist(
        seafile_rpc_client_,
        "seafile_get_file_sync_errors",
        SEAFILE_TYPE_FILE_SYNC_ERROR,
        &error, 2, "int", offset, "int", limit);


    for (GList *ptr = objlist; ptr; ptr = ptr->next) {
        SyncError error = SyncError::fromGObject((GObject *)ptr->data);
        errors->push_back(error);
    }

    g_list_foreach (objlist, (GFunc)g_object_unref, NULL);
    g_list_free (objlist);

    return true;
}

bool SeafileRpcClient::getSyncNotification(json_t **ret_obj)
{
    GError *error = NULL;
    json_t *ret = searpc_client_call__json (
        seafile_rpc_client_,
        "seafile_get_sync_notification",
        &error, 0);
    if (error) {
        qWarning("failed to get sync notification: %s\n",
                 error->message ? error->message : "");
        g_error_free(error);
        return false;
    }

    if (!ret) {
        // No pending notifications.
        return false;
    }

    *ret_obj = ret;

    return true;
}
