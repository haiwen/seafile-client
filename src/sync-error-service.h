#ifndef SEAFILE_CLIENT_SYNC_ERROR_H
#define SEAFILE_CLIENT_SYNC_ERROR_H

#include <QList>
#include <utility>

#include "utils/singleton.h"


struct sqlite3;
struct sqlite3_stmt;

/**
 * record the latest sync error id
 */
class LastSyncError {
SINGLETON_DEFINE(LastSyncError)
public:
    struct SyncErrorInfo {
        QString id;
    };

    void start();

    void saveLatestErrorID(const QString& id);

    QList<LastSyncError::SyncErrorInfo> getAllSyncErrorsInfo();

private:
    LastSyncError();
    ~LastSyncError();
    static bool collectSyncError(sqlite3_stmt *stmt, void *data);

    sqlite3 *db_;
};

#endif // SEAFILE_CLIENT_SYNC_ERROR_H
