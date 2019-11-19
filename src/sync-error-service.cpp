#include <QDir>
#include <sqlite3.h>

#include "utils/utils.h"
#include "configurator.h"
#include "seafile-applet.h"
#include "account-mgr.h"
#include "sync-error-service.h"

namespace {

    void sync_error_entry_from_sqlite3_result(sqlite3_stmt *stmt, LastSyncError::SyncErrorInfo* entry)
    {
        entry->id = sqlite3_column_int(stmt, 0);
    }

} // namespace


SINGLETON_IMPL(LastSyncError)
LastSyncError::LastSyncError()
{
   db_ = NULL;
}

LastSyncError::~LastSyncError()
{
    if (db_ != NULL)
        sqlite3_close(db_);
}

void LastSyncError::start()
{
    const char *errmsg;
    const char *sql;
    sqlite3 *db;

    QString db_path = QDir(seafApplet->configurator()->seafileDir()).filePath("sync_error.db");
    if (sqlite3_open (toCStr(db_path), &db)) {
        errmsg = sqlite3_errmsg (db);
        qDebug("failed to open sync error id database %s: %s",
               toCStr(db_path), errmsg ? errmsg : "no error given");

        seafApplet->errorAndExit(QObject::tr("failed to open sync error id database"));
        return;
    }

    sql = "CREATE TABLE IF NOT EXISTS SyncErrorID ("
          "     id integer NOT NULL, "
          "     accountsig VARCHAR(255) NOT NULL, "
          "     PRIMARY KEY (accountsig))";
    sqlite_query_exec (db, sql);

    db_ = db;
}

void LastSyncError::saveLatestErrorID(const int id)
{
    QString account_sig = seafApplet->accountManager()->currentAccount().getSignature();
    char *zql = sqlite3_mprintf("REPLACE INTO SyncErrorID(id, accountsig)"
                                " VALUES (%d, %Q)",
                                id,
                                account_sig.toUtf8().data());
    sqlite_query_exec(db_, zql);
    sqlite3_free(zql);
}

bool LastSyncError::collectSyncError(sqlite3_stmt *stmt, void *data)
{
    QList<SyncErrorInfo> *list = (QList<SyncErrorInfo> *)data;

    SyncErrorInfo entry;
    sync_error_entry_from_sqlite3_result(stmt, &entry);
    list->append(entry);
    return true;
}

int LastSyncError::getLastSyncErrorID() {
    QString account_sig = seafApplet->accountManager()->currentAccount().getSignature();
    char *sql = sqlite3_mprintf("SELECT *"
                                "  FROM SyncErrorID"
                                "  WHERE accountsig = %Q",
                                toCStr(account_sig));
    QList<SyncErrorInfo> list;
    sqlite_foreach_selected_row(db_, sql, collectSyncError, &list);
    if (list.size() >= 1) {
        return list[0].id;
    }
    return 0;
}

