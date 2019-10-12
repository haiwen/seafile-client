#ifndef SEAFILE_CLIENT_LOCAL_REPO_H
#define SEAFILE_CLIENT_LOCAL_REPO_H

#include <QString>
#include <QIcon>
#include <QMetaType>

#include "account.h"
#include "seafile/seafile-error.h"

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
        SYNC_STATE_UNKNOWN,
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
    Account account;

    qint64 last_sync_time;
    SyncState sync_state;
    QString rt_state;
    QString sync_state_str;
    QString sync_error_str;
    QString sync_error_detail;
    int transfer_percentage;
    int transfer_rate;
    bool has_data_transfer;

    LocalRepo()
        : encrypted(false),
        auto_sync(false),
        worktree_invalid(false),
        version(0),
        last_sync_time(0),
        sync_state(SYNC_STATE_DISABLED),
        has_data_transfer(false)
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
            && rt_state == rhs.rt_state
            && sync_state_str == rhs.sync_state_str
            && sync_error_str == rhs.sync_error_str
            && transfer_rate == rhs.transfer_rate
            && transfer_percentage == rhs.transfer_percentage;
    }

    bool operator!=(const LocalRepo& rhs) const {
        return !(*this == rhs);
    }

    bool isValid() const { return id.length() > 0; }

    QString getErrorString() const;

    void setSyncInfo(const QString &state, const int error = SYNC_ERROR_ID_NO_ERROR);

private:
    void translateSyncError(const int error);
    void translateSyncState(const QString &state);
};

Q_DECLARE_METATYPE(LocalRepo)

#endif // SEAFILE_CLIENT_LOCAL_REPO_H
