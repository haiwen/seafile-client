#include <QObject>
#include <QStringList>
#include <glib-object.h>

#include "utils/utils.h"
#include "sync-error.h"

SyncError SyncError::fromGObject(GObject *obj)
{
    SyncError error;

    char *repo_id = NULL;
    char *repo_name = NULL;
    char *path = NULL;
    int error_id = 0;
    qint64 timestamp = 0;

    g_object_get (obj,
                  "repo_id", &repo_id,
                  "repo_name", &repo_name,
                  "path", &path,
                  "err_id", &error_id,
                  "timestamp", &timestamp,
                  NULL);

    error.repo_id = repo_id;
    error.repo_name = QString::fromUtf8(repo_name);
    error.path = QString::fromUtf8(path);

    error.error_id = error_id;
    error.timestamp = timestamp;

    g_free (repo_id);
    g_free (repo_name);
    g_free (path);

    error.translateErrorStr();

    return error;
}

// Copied from seafile/daemon/repo-mgr.h
#define SYNC_ERROR_ID_FILE_LOCKED_BY_APP 0
#define SYNC_ERROR_ID_FOLDER_LOCKED_BY_APP 1
#define SYNC_ERROR_ID_FILE_LOCKED 2
#define SYNC_ERROR_ID_INVALID_PATH 3
#define SYNC_ERROR_ID_INDEX_ERROR 4
#define SYNC_ERROR_ID_PATH_END_SPACE_PERIOD 5
#define SYNC_ERROR_ID_PATH_INVALID_CHARACTER 6
#define SYNC_ERROR_ID_FOLDER_PERM_DENIED 7

void SyncError::translateErrorStr()
{
    readable_time_stamp = translateCommitTime(timestamp);

    switch (error_id) {
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
    default:
        // unreachable
        qWarning("unknown sync error id %d", error_id);
        error_str = "";
    }
}
