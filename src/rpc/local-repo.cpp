#include <glib-object.h>

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

void LocalRepo::setSyncInfo(const QString &state, const QString &error, const QString &err_detail)
{
    // qWarning("error: %s\n", toCStr(error));
    // qWarning("state: %s\n", toCStr(state));
    if (error.length() > 0) {
        translateSyncError(error);
        if (err_detail.length() > 0)
            translateSyncErrDetail(err_detail);
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

void LocalRepo::translateSyncError(const QString &error)
{
    sync_state = SYNC_STATE_ERROR;

    if (error == "relay not connected") {
        sync_error_str = QObject::tr("server not connected");

    } else if (error == "Server has been removed") {
        sync_error_str = QObject::tr("Server has been removed");

    } else if (error == "You have not login to the server") {
        sync_error_str = QObject::tr("You have not logged in to the server");

    } else if (error == "You do not have permission to access this repo") {
        sync_error_str = QObject::tr("You do not have permission to access this library");

    } else if (error == "The storage space of the repo owner has been used up") {
        sync_error_str = QObject::tr("The storage space of the library owner has been used up");

    } else if (error == "Remote service is not available") {
        sync_error_str = QObject::tr("Remote service is not available");

    } else if (error == "Access denied to service. Please check your registration on relay.") {
        sync_error_str = QObject::tr("Access denied to service");

    } else if (error == "Internal data corrupted.") {
        sync_error_str = QObject::tr("Internal data corrupted");

    } else if (error == "Failed to start upload.") {
        sync_error_str = QObject::tr("Failed to start upload");

    } else if (error == "Error occurred in upload.") {
        sync_error_str = QObject::tr("Error occurred in upload");

    } else if (error == "Failed to start download.") {
        sync_error_str = QObject::tr("Failed to start download");

    } else if (error == "Error occurred in download.") {
        sync_error_str = QObject::tr("Error occurred in download");

    } else if (error == "No such repo on relay.") {
        sync_error_str = QObject::tr("Library is deleted on server");

    } else if (error == "Repo is damaged on relay.") {
        sync_error_str = QObject::tr("Library is damaged on server");

    } else if (error == "Conflict in merge.") {
        sync_error_str = QObject::tr("Conflict in merge");

    } else if (error == "Server version is too old.") {
        sync_error_str = QObject::tr("Server version is too old");

    } else if (error == "invalid worktree") {
        sync_error_str = QObject::tr("Error when accessing the local folder");

    } else if (error == "Unknown error." || error == "Unknown error") {
        sync_error_str = QObject::tr("Unknown error");

    } else if (error == "Storage quota full") {
        sync_error_str = QObject::tr("The storage quota has been used up");

    } else if (error == "Service on remote server is not available") {
        sync_error_str = QObject::tr("Internal server error");

    } else if (error == "Access denied to service. Please check your registration on server.") {
        sync_error_str = QObject::tr("Access denied to service");

    } else if (error == "Transfer protocol outdated. You need to upgrade seafile.") {
        sync_error_str = QObject::tr("Your %1 client is too old").arg(getBrand());

    } else if (error == "Internal error when preparing upload") {
        sync_error_str = QObject::tr("Failed to sync this library");

    } else if (error == "Internal error when preparing download") {
        sync_error_str = QObject::tr("Failed to sync this library");

    } else if (error == "No permission to access remote library") {
        sync_error_str = QObject::tr("Failed to sync this library");

    } else if (error == "Library doesn't exist on the remote end") {
        sync_error_str = QObject::tr("Failed to sync this library");

    } else if (error == "Internal error when starting to send revision information") {
        sync_error_str = QObject::tr("Failed to sync this library");
    } else if (error == "Internal error when starting to get revision information") {
        sync_error_str = QObject::tr("Failed to sync this library");
    } else if (error == "Failed to upload revision information to remote library") {
        sync_error_str = QObject::tr("Failed to sync this library");
    } else if (error == "Failed to get revision information from remote library") {
        sync_error_str = QObject::tr("Failed to sync this library");
    } else if (error == "Internal error when starting to send file information") {
        sync_error_str = QObject::tr("Failed to sync this library");
    } else if (error == "Internal error when starting to get file information") {
        sync_error_str = QObject::tr("Failed to sync this library");
    } else if (error == "Incomplete file information in the local library") {
        sync_error_str = QObject::tr("Failed to sync this library");
    } else if (error == "Failed to upload file information to remote library") {
        sync_error_str = QObject::tr("Failed to sync this library");
    } else if (error == "Failed to get file information from remote library") {
        sync_error_str = QObject::tr("Failed to sync this library");
    } else if (error == "Internal error when starting to update remote library") {
        sync_error_str = QObject::tr("Failed to sync this library");
    } else if (error == "Others have concurrent updates to the remote library. You need to sync again.") {
        sync_error_str = QObject::tr("Failed to sync this library");
    } else if (error == "Server failed to check storage quota") {
        sync_error_str = QObject::tr("Failed to sync this library");
    } else if (error == "Incomplete revision information in the local library") {
        sync_error_str = QObject::tr("Failed to sync this library");
    } else if (error == "Failed to compare data to server.") {
        sync_error_str = QObject::tr("Failed to sync this library");
    } else if (error == "Failed to get block server list.") {
        sync_error_str = QObject::tr("Failed to sync this library");
    } else if (error == "Failed to start block transfer client.") {
        sync_error_str = QObject::tr("Failed to sync this library");
    } else if (error == "Failed to upload blocks.") {
        sync_error_str = QObject::tr("Failed to sync this library");
    } else if (error == "Failed to download blocks.") {
        sync_error_str = QObject::tr("Failed to sync this library");

    } else if (error == "Files are locked by other application") {
        sync_error_str = QObject::tr("Files are locked by other application");
    } else {
        sync_error_str = error;
    }
}

void LocalRepo::translateSyncErrDetail(const QString &err_detail)
{
    if (err_detail == "Permission denied on server") {
        sync_error_detail = QObject::tr("Permission denied on server. Please try resync the library");
    } else if (err_detail == "Network error") {
        sync_error_detail = QObject::tr("Network error");
    } else if (err_detail == "Cannot resolve proxy address") {
        sync_error_detail = QObject::tr("Cannot resolve proxy address");
    } else if (err_detail == "Cannot resolve server address") {
        sync_error_detail = QObject::tr("Cannot resolve server address");
    } else if (err_detail == "Cannot connect to server") {
        sync_error_detail = QObject::tr("Cannot connect to server");
    } else if (err_detail == "Failed to establish secure connection") {
        sync_error_detail = QObject::tr("Failed to establish secure connection. Please check server SSL certificate");
    } else if (err_detail == "Data transfer was interrupted") {
        sync_error_detail = QObject::tr("Data transfer was interrupted. Please check network or firewall");
    } else if (err_detail == "Data transfer timed out") {
        sync_error_detail = QObject::tr("Data transfer timed out. Please check network or firewall");
    } else if (err_detail == "Unhandled http redirect from server") {
        sync_error_detail = QObject::tr("Unhandled http redirect from server. Please check server cofiguration");
    } else if (err_detail == "Server error") {
        sync_error_detail = QObject::tr("Server error");
    } else if (err_detail == "Bad request") {
        sync_error_detail = QObject::tr("Bad request");
    } else if (err_detail == "Internal data corrupt on the client") {
        sync_error_detail = QObject::tr("Internal data corrupt on the client. Please try resync the library");
    } else if (err_detail == "Not enough memory") {
        sync_error_detail = QObject::tr("Not enough memory");
    } else if (err_detail == "Failed to write data on the client") {
        sync_error_detail = QObject::tr("Failed to write data on the client. Please check disk space or folder permissions");
    } else if (err_detail == "Storage quota full") {
        sync_error_detail = QObject::tr("Storage quota full");
    } else if (err_detail == "Files are locked by other application") {
        sync_error_detail = QObject::tr("Files are locked by other application");
    } else if (err_detail == "Library deleted on server") {
        sync_error_detail = QObject::tr("Library deleted on server");
    } else if (err_detail == "Library damaged on server") {
        sync_error_detail = QObject::tr("Library damaged on server");
    } else if (err_detail == "File is locked by another user") {
        sync_error_detail = QObject::tr("File is locked by another user");
    } else {
        sync_error_detail = err_detail;
    }

    // printf ("sync error detail set to '%s'\n", sync_error_detail.toUtf8().data());
}

QString LocalRepo::getErrorString() const
{
    return sync_error_detail.isEmpty() ? sync_error_str : sync_error_detail;
}
