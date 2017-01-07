#ifndef SEAFILE_CLIENT_RPC_SYNC_ERROR_H
#define SEAFILE_CLIENT_RPC_SYNC_ERROR_H

#include <QString>
#include <QMetaType>

struct _GObject;

class SyncError {
public:
    QString repo_id;
    QString repo_name;
    QString path;
    qint64 timestamp;
    int error_id;

    // Generated fields.
    QString readable_time_stamp;
    QString error_str;

    static SyncError fromGObject(_GObject *obj);

    void translateErrorStr();

    bool operator==(const SyncError& rhs) const {
        return repo_id == rhs.repo_id
            && repo_name == rhs.repo_name
            && path == rhs.path
            && error_id == rhs.error_id
            && timestamp == rhs.timestamp;
    }

    bool operator!=(const SyncError& rhs) const {
        return !(*this == rhs);
    }
};

#endif // SEAFILE_CLIENT_RPC_SYNC_ERROR_H
