#include <QObject>
#include <QStringList>
#include <glib-object.h>

#include "utils/utils.h"
#include "sync-error.h"
#include "utils/seafile-error.h"

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

// SyncError only include file level and repository level
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
    case SYNC_ERROR_ID_PERM_NOT_SYNCABLE:
        error_str = QObject::tr("No permission to sync this folder");
        break;
    case SYNC_ERROR_ID_UPDATE_TO_READ_ONLY_REPO:
        error_str = QObject::tr("Updates in read-only library will not be uploaded");
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
        error_str = QObject::tr("Unkown error");
        break;
    default:
        // unreachable
        qWarning("unknown sync error id %d", error_id);
    }
}
