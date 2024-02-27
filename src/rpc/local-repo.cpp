#include <glib-object.h>
#include <ui/tray-icon.h>

#include "utils/utils.h"
#include "utils/seafile-error.h"
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
    if (error == SYNC_ERROR_ID_NO_ERROR) {
        translateSyncState(state);
    } else {
        translateSyncError(error);
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
    sync_error_str = translateSyncErrorCode(error);
    sync_state = SYNC_STATE_ERROR;
}

QString LocalRepo::getErrorString() const
{
    return sync_error_str;
}
