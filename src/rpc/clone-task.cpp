#include <QObject>
#include <QStringList>
#include <glib-object.h>

#include "utils/utils.h"
#include "clone-task.h"
#include "utils/seafile-error.h"

CloneTask CloneTask::fromGObject(GObject *obj)
{
    CloneTask task;

    char *state = NULL;
    int error_code = 0;
    char *repo_id = NULL;
    char *peer_id = NULL;
    char *repo_name = NULL;
    char *worktree = NULL;
    char *tx_id = NULL;

    g_object_get (obj,
                  "state", &state,
                  "error", &error_code,
                  "repo_id", &repo_id,
                  "peer_id", &peer_id,
                  "repo_name", &repo_name,
                  "worktree", &worktree,
                  "tx_id", &tx_id,
                  NULL);

    task.state = QString::fromUtf8(state);
    task.error_code = error_code;
    task.repo_id = QString::fromUtf8(repo_id);
    task.peer_id = QString::fromUtf8(peer_id);
    task.repo_name = QString::fromUtf8(repo_name);
    task.worktree = QString::fromUtf8(worktree);
    task.tx_id = QString::fromUtf8(tx_id);

    task.block_done = 0;
    task.block_total = 0;

    task.checkout_done = 0;
    task.checkout_total = 0;

    g_free (state);
    g_free (repo_id);
    g_free (peer_id);
    g_free (repo_name);
    g_free (worktree);
    g_free (tx_id);

    // task.translateStateInfo();

    return task;
}

QString CloneTask::calcProgress(int done, int total)
{
    if (total == 0) {
        return QString();
    }

    int percentage = done * 100 / total;

    return QString().sprintf(" %d%%", percentage);
}

void CloneTask::translateStateInfo()
{
    if (state == "init") {
        state_str = QObject::tr("initializing...");

    } else if (state == "check server") {
        state_str = QObject::tr("checking server info...");

    } else if (state == "fetch") {
        if (rt_state == "fs") {
            state_str = QObject::tr("Downloading file list...");
            if (fs_objects_total != 0) {
                state_str += calcProgress(fs_objects_done, fs_objects_total);
            }
        } else if (rt_state == "data") {
            state_str = QObject::tr("Downloading files...");
            if (block_total != 0) {
                state_str += calcProgress(block_done, block_total);
            }
        }

    } else if (state == "done") {
        state_str = QObject::tr("Done");

    } else if (state == "canceling") {
        state_str = QObject::tr("Canceling");

    } else if (state == "canceled") {
        state_str = QObject::tr("Canceled");

    } else if (state == "error") {
        switch (error_code) {
        case SYNC_ERROR_ID_FILE_LOCKED_BY_APP:
            error_str = QObject::tr("File is locked by another application");
            break;
        case SYNC_ERROR_ID_FOLDER_LOCKED_BY_APP:
            error_str = QObject::tr("Folder is locked by another application");
            break;
        case SYNC_ERROR_ID_FILE_LOCKED:
            error_str = QObject::tr("File is locked by another user");
            break;
        case SYNC_ERROR_ID_INVALID_PATH:
            error_str = QObject::tr("Path is invalid");
            break;
        case SYNC_ERROR_ID_INDEX_ERROR:
            error_str = QObject::tr("Error when indexing");
            break;
        case SYNC_ERROR_ID_PATH_END_SPACE_PERIOD:
            error_str = QObject::tr("Path ends with space or period character");
            break;
        case SYNC_ERROR_ID_PATH_INVALID_CHARACTER:
            error_str = QObject::tr("Path contains invalid characters like '|' or ':'");
            break;
        case SYNC_ERROR_ID_FOLDER_PERM_DENIED:
            error_str = QObject::tr("Update to file denied by folder permission setting");
            break;
        case SYNC_ERROR_ID_PERM_NOT_SYNCABLE:
            error_str = QObject::tr("No permission to sync this folder");
            break;
        case SYNC_ERROR_ID_UPDATE_TO_READ_ONLY_REPO:
            error_str = QObject::tr("Created or updated a file in a non-writable library or folder");
            break;
        case SYNC_ERROR_ID_ACCESS_DENIED:
            error_str = QObject::tr("Permission denied on server");
            break;
        case SYNC_ERROR_ID_NO_WRITE_PERMISSION:
            error_str = QObject::tr("Do not have write permission to the library");
            break;
        case SYNC_ERROR_ID_QUOTA_FULL:
            error_str = QObject::tr("Storage quota full");
            break;
        case SYNC_ERROR_ID_NETWORK:
            error_str = QObject::tr("Network error");
            break;
        case SYNC_ERROR_ID_RESOLVE_PROXY:
            error_str = QObject::tr("Cannot resolve proxy address");
            break;
        case SYNC_ERROR_ID_RESOLVE_HOST:
            error_str = QObject::tr("Cannot resolve server address");
            break;
        case SYNC_ERROR_ID_CONNECT:
            error_str = QObject::tr("Cannot connect to server");
            break;
        case SYNC_ERROR_ID_SSL:
            error_str = QObject::tr("Failed to establish secure connection. Please check server SSL certificate");
            break;
        case SYNC_ERROR_ID_TX:
            error_str = QObject::tr("Data transfer was interrupted. Please check network or firewall");
            break;
        case SYNC_ERROR_ID_TX_TIMEOUT:
            error_str = QObject::tr("Data transfer timed out. Please check network or firewall");
            break;
        case SYNC_ERROR_ID_UNHANDLED_REDIRECT:
            error_str = QObject::tr("Unhandled http redirect from server. Please check server cofiguration");
            break;
        case SYNC_ERROR_ID_SERVER:
            error_str = QObject::tr("Server error");
            break;
        case SYNC_ERROR_ID_LOCAL_DATA_CORRUPT:
            error_str = QObject::tr("Internal data corrupt on the client. Please try to resync the library");
            break;
        case SYNC_ERROR_ID_WRITE_LOCAL_DATA:
            error_str = QObject::tr("Failed to write data on the client. Please check disk space or folder permissions");
            break;
        case SYNC_ERROR_ID_SERVER_REPO_DELETED:
            error_str = QObject::tr("Library deleted on server");
            break;
        case SYNC_ERROR_ID_SERVER_REPO_CORRUPT:
            error_str = QObject::tr("Library damaged on server");
            break;
        case SYNC_ERROR_ID_NOT_ENOUGH_MEMORY:
            error_str = QObject::tr("Not enough memory");
            break;
        case SYNC_ERROR_ID_CONFLICT:
            error_str = QObject::tr("Concurrent updates to file. File is saved as conflict file");
            break;
        case SYNC_ERROR_ID_GENERAL_ERROR:
            error_str = QObject::tr("Unknown error");
            break;
        case SYNC_ERROR_ID_NO_ERROR:
            error_str = QObject::tr("No error");
            break;
        default:
            qWarning("Unknown sync error");
        }

    } else if (state == "connect") {
        state_str = QObject::tr("connecting server...");

    } else if (state == "index") {
        state_str = QObject::tr("indexing files...");

    } else if (state == "checkout") {
        state_str = QObject::tr("Creating folder...");
        if (checkout_total != 0) {
            state_str += calcProgress(checkout_done, checkout_total);
        }

    } else if (state == "merge") {
        state_str = QObject::tr("Merge file changes...");
    }

    if (state != "error" || error_str == "ok") {
        error_str = "";
    }
}

bool CloneTask::isCancelable() const
{
    QStringList list;
    list << "init" << "connect" << "index" << "fetch" << "error";
    return list.contains(state);
}


bool CloneTask::isRemovable() const
{
    QStringList list;
    list << "done" << "canceled";
    return list.contains(state);
}

bool CloneTask::isDisplayable() const
{
    return state != "canceled" && state != "done";
}
