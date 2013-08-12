#ifndef SEAFILE_CLIENT_LOCAL_REPO_H
#define SEAFILE_CLIENT_LOCAL_REPO_H

#include <QString>

struct _GObject;

/**
 * Repo Information from local seaf-daemon
 */
class LocalRepo {
public:
    QString id;
    QString name;
    QString description;
    QString worktree;
    bool encrypted;
    bool auto_sync;

    qint64 last_sync_time;

    static LocalRepo fromGObject(_GObject *obj);
};

#endif // SEAFILE_CLIENT_LOCAL_REPO_H
