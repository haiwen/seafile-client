#include <glib-object.h>
#include <src/ui/tray-icon.h>

#include "utils/utils.h"
#include "seafile-applet.h"
#include "local-repo.h"

LocalRepo LocalRepo::fromGObject(GObject *obj)
{
    char *id = NULL;
    char *name = NULL;
    char *desc = NULL;
    char *worktree = NULL;
    char *relay_id = NULL;

    gboolean encrypted;
    gboolean auto_sync;
    gboolean worktree_invalid;
    gint64 last_sync_time;
    int version;

    g_object_get (obj,
                  "id", &id,
                  "name", &name,
                  "desc", &desc,
                  "encrypted", &encrypted,
                  "worktree", &worktree,
                  "auto-sync", &auto_sync,
                  "last-sync-time", &last_sync_time,
                  "worktree-invalid", &worktree_invalid,
                  "relay-id", &relay_id,
                  "version", &version,
                  NULL);

    LocalRepo repo;
    repo.id = QString::fromUtf8(id);
    repo.name = QString::fromUtf8(name);
    repo.description = QString::fromUtf8(desc);
    repo.worktree = QString::fromUtf8(worktree);
    repo.encrypted = encrypted;
    repo.auto_sync = auto_sync;
    repo.last_sync_time = last_sync_time;
    repo.worktree_invalid = worktree_invalid;
    repo.relay_id = relay_id;
    repo.version = version;

    g_free (id);
    g_free (name);
    g_free (desc);
    g_free (worktree);
    g_free (relay_id);

    return repo;
}

void LocalRepo::setSyncInfo(const QString &state, const int error)
{
    // qWarning("error: %s\n", toCStr(error));
    // qWarning("state: %s\n", toCStr(state));
    if (error != SYNC_ERROR_ID_NO_ERROR) {
        translateSyncError(error);
    } else {
        translateSyncState(state);
    }
}

void LocalRepo::translateSyncState(const QString &status)
{
    if (status == "synchronized") {
        sync_state_str = QObject::tr("synchronized");
        sync_state = SYNC_STATE_DONE;

    } else if (status == "committing") {
        sync_state_str = QObject::tr("indexing files");
        sync_state = SYNC_STATE_ING;
        has_data_transfer = false;

    } else if (status == "initializing") {
        sync_state_str = QObject::tr("sync initializing");
        sync_state = SYNC_STATE_INIT;

    } else if (status == "downloading") {
        sync_state_str = QObject::tr("downloading");
        sync_state = SYNC_STATE_ING;
        has_data_transfer = true;

    } else if (status == "uploading") {
        sync_state_str = QObject::tr("uploading");
        sync_state = SYNC_STATE_ING;
        has_data_transfer = true;

    } else if (status == "merging") {
        sync_state_str = QObject::tr("sync merging");
        sync_state = SYNC_STATE_ING;
        has_data_transfer = false;

    } else if (status == "waiting for sync") {
        sync_state_str = QObject::tr("waiting for sync");
        sync_state = SYNC_STATE_WAITING;

    } else if (status == "relay not connected") {
        sync_state_str = QObject::tr("server not connected");
        sync_state = SYNC_STATE_WAITING;

    } else if (status == "relay authenticating") {
        sync_state_str = QObject::tr("server authenticating");
        sync_state = SYNC_STATE_WAITING;

    } else if (status == "auto sync is turned off") {
        sync_state_str = QObject::tr("auto sync is turned off");
        sync_state = SYNC_STATE_DISABLED;

    } else if (status == "cancel pending") {
        sync_state_str = QObject::tr("sync initializing");
        sync_state = SYNC_STATE_INIT;

    } else {
        qWarning("unknown sync status: %s\n", toCStr(status));
        sync_state_str = QObject::tr("unknown");
        sync_state = SYNC_STATE_UNKNOWN;
    }

    if (!auto_sync && sync_state != SYNC_STATE_ING) {
        sync_state_str = QObject::tr("auto sync is turned off");
        sync_state = SYNC_STATE_DISABLED;
        return;
    }
}

void LocalRepo::translateSyncError(const int error)
{
    sync_state = SYNC_STATE_ERROR;
    // When sync error set trayIcon to warning status
    seafApplet->trayIcon()->setState(SeafileTrayIcon::TrayState::STATE_SERVERS_NOT_CONNECTED);

    switch (error) {
    case SYNC_ERROR_ID_FILE_LOCKED_BY_APP:
        sync_state_str = QObject::tr("File is locked by another application");
        break;
    case SYNC_ERROR_ID_FOLDER_LOCKED_BY_APP:
        sync_state_str = QObject::tr("Folder is locked by another application");
        break;
    case SYNC_ERROR_ID_FILE_LOCKED:
        sync_state_str = QObject::tr("File is locked by another user");
        break;
    case SYNC_ERROR_ID_INVALID_PATH:
        sync_state_str = QObject::tr("Path is invalid");
        break;
    case SYNC_ERROR_ID_INDEX_ERROR:
        sync_state_str = QObject::tr("Error when indexing");
        break;
    case SYNC_ERROR_ID_PATH_END_SPACE_PERIOD:
        sync_state_str = QObject::tr("Path ends with space or period character");
        break;
    case SYNC_ERROR_ID_PATH_INVALID_CHARACTER:
        sync_state_str = QObject::tr("Path contains invalid characters like '|' or ':'");
        break;
    case SYNC_ERROR_ID_FOLDER_PERM_DENIED:
        sync_state_str = QObject::tr("Update to file denied by folder permission setting");
        break;
    case SYNC_ERROR_ID_PERM_NOT_SYNCABLE:
        sync_state_str = QObject::tr("No permission to sync this folder");
        break;
    case SYNC_ERROR_ID_UPDATE_TO_READ_ONLY_REPO:
        sync_state_str = QObject::tr("Created or updated a file in a non-writable library or folder");
        break;
    case SYNC_ERROR_ID_ACCESS_DENIED:
        sync_state_str = QObject::tr("Permission denied on server");
        break;
    case SYNC_ERROR_ID_NO_WRITE_PERMISSION:
        sync_state_str = QObject::tr("Do not have write permission to the library");
        break;
    case SYNC_ERROR_ID_QUOTA_FULL:
        sync_state_str = QObject::tr("Storage quota full");
        break;
    case SYNC_ERROR_ID_NETWORK:
        sync_state_str = QObject::tr("Network error");
        break;
    case SYNC_ERROR_ID_RESOLVE_PROXY:
        sync_state_str = QObject::tr("Cannot resolve proxy address");
        break;
    case SYNC_ERROR_ID_RESOLVE_HOST:
        sync_state_str = QObject::tr("Cannot resolve server address");
        break;
    case SYNC_ERROR_ID_CONNECT:
        sync_state_str = QObject::tr("Cannot connect to server");
        break;
    case SYNC_ERROR_ID_SSL:
        sync_state_str = QObject::tr("Failed to establish secure connection. Please check server SSL certificate");
        break;
    case SYNC_ERROR_ID_TX:
        sync_state_str = QObject::tr("Data transfer was interrupted. Please check network or firewall");
        break;
    case SYNC_ERROR_ID_TX_TIMEOUT:
        sync_state_str = QObject::tr("Data transfer timed out. Please check network or firewall");
        break;
    case SYNC_ERROR_ID_UNHANDLED_REDIRECT:
        sync_state_str = QObject::tr("Unhandled http redirect from server. Please check server cofiguration");
        break;
    case SYNC_ERROR_ID_SERVER:
        sync_state_str = QObject::tr("Server error");
        break;
    case SYNC_ERROR_ID_LOCAL_DATA_CORRUPT:
        sync_state_str = QObject::tr("Internal data corrupt on the client. Please try to resync the library");
        break;
    case SYNC_ERROR_ID_WRITE_LOCAL_DATA:
        sync_state_str = QObject::tr("Failed to write data on the client. Please check disk space or folder permissions");
        break;
    case SYNC_ERROR_ID_SERVER_REPO_DELETED:
        sync_state_str = QObject::tr("Library deleted on server");
        break;
    case SYNC_ERROR_ID_SERVER_REPO_CORRUPT:
        sync_state_str = QObject::tr("Library damaged on server");
        break;
    case SYNC_ERROR_ID_NOT_ENOUGH_MEMORY:
        sync_state_str = QObject::tr("Not enough memory");
        break;
    case SYNC_ERROR_ID_CONFLICT:
        sync_state_str = QObject::tr("Concurrent updates to file. File is saved as conflict file");
        break;
    case SYNC_ERROR_ID_GENERAL_ERROR:
        sync_state_str = QObject::tr("Unknown error");
        break;
    case SYNC_ERROR_ID_NO_ERROR:
        sync_state_str = QObject::tr("No error");
        break;
    default:
        qWarning("Unknown sync error");
    }
}


QString LocalRepo::getErrorString() const
{
    return  sync_error_str;
}
