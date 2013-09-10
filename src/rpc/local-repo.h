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
    enum SyncState {
        SYNC_STATE_DISABLED,
        SYNC_STATE_WAITING,
        SYNC_STATE_ING,
        SYNC_STATE_DONE,
        SYNC_STATE_ERROR,
        SYNC_STATE_UNKNOWN,
    };

    QString id;
    QString name;
    QString description;
    QString worktree;
    bool encrypted;
    bool auto_sync;

    qint64 last_sync_time;
    SyncState sync_state;
    QString sync_state_str;
    QString sync_error_str;

    static LocalRepo fromGObject(_GObject *obj);

    bool operator==(const LocalRepo& rhs) const {
        return id == rhs.id
            && name == rhs.name
            && description == rhs.description
            && worktree == rhs.worktree
            && encrypted == rhs.encrypted
            && auto_sync == rhs.auto_sync
            && last_sync_time == rhs.last_sync_time
            && sync_state == rhs.sync_state
            && sync_state_str == rhs.sync_state_str
            && sync_error_str == rhs.sync_error_str;
    }

    bool operator!=(const LocalRepo& rhs) const {
        return !(*this == rhs);
    }

    QIcon getIcon();
    void setSyncInfo(QString state, QString error = QString());

private:
    void translateSyncError(QString error);
    void translateSyncState(QString state);
};

#endif // SEAFILE_CLIENT_LOCAL_REPO_H
