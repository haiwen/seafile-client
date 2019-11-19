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
        int id;
    };

    void start();

    void saveLatestErrorID(const int id);
    int getLastSyncErrorID();

    QList<LastSyncError::SyncErrorInfo> getAllSyncErrorsInfo();

private:
    LastSyncError();
    ~LastSyncError();
    static bool collectSyncError(sqlite3_stmt *stmt, void *data);

    sqlite3 *db_;
};

#endif // SEAFILE_CLIENT_SYNC_ERROR_H
