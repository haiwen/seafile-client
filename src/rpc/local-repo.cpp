#include <glib-object.h>
#include "local-repo.h"

LocalRepo LocalRepo::fromGObject(GObject *obj)
{
    char *id = NULL;
    char *name = NULL;
    char *desc = NULL;
    char *worktree = NULL;

    bool encrypted;
    bool auto_sync;
    gint64 last_sync_time;

    g_object_get (obj,
                  "id", &id,
                  "name", &name,
                  "desc", &desc,
                  "encrypted", &encrypted,
                  "worktree", &worktree,
                  "auto-sync", &auto_sync,
                  "last-sync-time", &last_sync_time,
                  NULL);

    LocalRepo repo;
    repo.id = QString::fromUtf8(id);
    repo.name = QString::fromUtf8(name);
    repo.description = QString::fromUtf8(desc);
    repo.encrypted = encrypted;
    repo.auto_sync = auto_sync;
    repo.last_sync_time = last_sync_time;

    g_free (id);
    g_free (name);
    g_free (desc);
    g_free (worktree);

    return repo;
}

