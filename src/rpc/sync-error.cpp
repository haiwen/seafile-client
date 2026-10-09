#include <QObject>
#include <QStringList>
#include <glib-object.h>

#include "utils/utils.h"
#include "sync-error.h"
#include "utils/seafile-error.h"

SyncError SyncError::fromGObject(GObject *obj)
{
    SyncError error;

    int id  = 0;
    char *server = NULL;
    char *repo_id = NULL;
    char *repo_name = NULL;
    char *path = NULL;
    int error_id = 0;
    qint64 timestamp = 0;

    g_object_get (obj,
                  "id", &id,
                  "server", &server,
                  "repo_id", &repo_id,
                  "repo_name", &repo_name,
                  "path", &path,
                  "err_id", &error_id,
                  "timestamp", &timestamp,
                  NULL);

    error.id = id;
    error.server = QString::fromUtf8(server);
    error.repo_id = repo_id;
    error.repo_name = QString::fromUtf8(repo_name);
    error.path = QString::fromUtf8(path);

    error.error_id = error_id;
    error.timestamp = timestamp;

    g_free (server);
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
