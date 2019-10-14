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

    error_str = translateSyncErrorCode(error_id);
}
