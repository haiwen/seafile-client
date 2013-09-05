#ifndef SEAFILE_CLIENT_LOCAL_REPO_H
#define SEAFILE_CLIENT_LOCAL_REPO_H

#include <QString>
#include <QIcon>

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

    bool operator==(const LocalRepo& rhs) const {
        return id == rhs.id
            && name == rhs.name
            && description == rhs.description
            && worktree == rhs.worktree
            && encrypted == rhs.encrypted
            && auto_sync == rhs.auto_sync
            && last_sync_time == rhs.last_sync_time;
    }

    bool operator!=(const LocalRepo& rhs) const {
        return !(*this == rhs);
    }

    QIcon getIcon();
};

#endif // SEAFILE_CLIENT_LOCAL_REPO_H
