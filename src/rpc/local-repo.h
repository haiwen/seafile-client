#ifndef SEAFILE_CLIENT_LOCAL_REPO_H
#define SEAFILE_CLIENT_LOCAL_REPO_H

#include <QString>
#include <QIcon>
#include <QMetaType>

struct _GObject;

/**
 * Repo Information from local seaf-daemon
 */
class LocalRepo {
public:
    enum SyncState {
        SYNC_STATE_DISABLED,
        SYNC_STATE_WAITING,
        SYNC_STATE_INIT,
        SYNC_STATE_ING,
        SYNC_STATE_DONE,
        SYNC_STATE_ERROR,
        SYNC_STATE_UNKNOWN
    };

    QString id;
    QString name;
    QString description;
    QString worktree;
    bool encrypted;
    bool auto_sync;
    bool worktree_invalid;
    int version;
    QString relay_id;

    qint64 last_sync_time;
    SyncState sync_state;
    QString sync_state_str;
    QString sync_error_str;

    LocalRepo()
        : encrypted(false),
        auto_sync(false),
        worktree_invalid(false),
        version(0),
        last_sync_time(0),
        sync_state(SYNC_STATE_DISABLED)
    {
    }

    static LocalRepo fromGObject(_GObject *obj);

    bool operator==(const LocalRepo& rhs) const {
        return id == rhs.id
            && name == rhs.name
            && description == rhs.description
            && worktree == rhs.worktree
            && encrypted == rhs.encrypted
            && auto_sync == rhs.auto_sync
            && sync_state == rhs.sync_state
            && sync_state_str == rhs.sync_state_str
            && sync_error_str == rhs.sync_error_str;
    }

    bool operator!=(const LocalRepo& rhs) const {
        return !(*this == rhs);
    }

    bool isValid() const { return id.length() > 0; }

    void setSyncInfo(const QString &state, const QString &error = QString());

private:
    void translateSyncError(const QString &error);
    void translateSyncState(const QString &state);
};

Q_DECLARE_METATYPE(LocalRepo)

#endif // SEAFILE_CLIENT_LOCAL_REPO_H
