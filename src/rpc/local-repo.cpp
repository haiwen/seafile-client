#include <glib-object.h>
#include "local-repo.h"
extern "C" {
#include <searpc-client.h>
#include <ccnet.h>
    
#include <seafile/seafile.h>
#include <seafile/seafile-object.h>
    
}

LocalRepo LocalRepo::fromGObject(GObject *obj)
{
    char *id = NULL;
    char *name = NULL;
    char *desc = NULL;
    char *worktree = NULL;

    gboolean encrypted;
    gboolean auto_sync;
    gint64 last_sync_time;

    //qDebug("#%d %s %d:%d\n", __LINE__, __func__, G_IS_OBJECT(obj), SEAFILE_IS_REPO(obj));
    g_object_get (obj,
                  "id", &id,
                  "name", &name,
                  "desc", &desc,
                  "encrypted", &encrypted,
                  "worktree", &worktree,
                  "auto-sync", &auto_sync,
                  "last-sync-time", &last_sync_time,
                  NULL);
    //qDebug("#%d %s\n", __LINE__, __func__);

    LocalRepo repo;
    repo.id = QString::fromUtf8(id);
    repo.name = QString::fromUtf8(name);
    repo.description = QString::fromUtf8(desc);
    repo.worktree = QString::fromUtf8(worktree);
    repo.encrypted = encrypted;
    repo.auto_sync = auto_sync;
    repo.last_sync_time = last_sync_time;

    g_free (id);
    g_free (name);
    g_free (desc);
    g_free (worktree);

    return repo;
}

QIcon LocalRepo::getIcon() {
    return QIcon(":/images/repo.svg");
}
