#include <QObject>
#include <QStringList>
#include <glib-object.h>

#include "utils/utils.h"
#include "sync-error.h"
#include "utils/seafile-error.h"
#if defined(_MSC_VER)
#include "include/seafile-error.h"
#else
#include <seafile/seafile-error.h>
#endif

SyncError SyncError::fromGObject(GObject *obj)
{
    SyncError error;

    int id  = 0;
    char *repo_id = NULL;
    char *repo_name = NULL;
    char *path = NULL;
    int error_id = 0;
    qint64 timestamp = 0;

    g_object_get (obj,
                  "id", &id,
                  "repo_id", &repo_id,
                  "repo_name", &repo_name,
                  "path", &path,
                  "err_id", &error_id,
                  "timestamp", &timestamp,
                  NULL);

    error.id = id;
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
    error_str = translateSyncErrorCode(error_id);
}

bool SyncError::isNetworkError() const
{
    switch (error_id) {
    case SYNC_ERROR_ID_NETWORK:
    case SYNC_ERROR_ID_RESOLVE_PROXY:
    case SYNC_ERROR_ID_RESOLVE_HOST:
    case SYNC_ERROR_ID_CONNECT:
    case SYNC_ERROR_ID_SSL:
    case SYNC_ERROR_ID_TX:
    case SYNC_ERROR_ID_TX_TIMEOUT:
    case SYNC_ERROR_ID_UNHANDLED_REDIRECT:
        return true;
    default:
        return false;
    }
}
