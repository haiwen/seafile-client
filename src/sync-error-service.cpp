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

    // RepoSyncError table records special type errors, which will be persistent displayed on the RepoTreeView.
    sql = "CREATE TABLE IF NOT EXISTS RepoSyncError ("
          "    accountsig text NOT NULL,"
          "    repoid text NOT NULL,"
          "    errid integer NOT NULL,"
          "    PRIMARY KEY (accountsig, repoid)"
          ")";
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

bool LastSyncError::collectRepoSyncError(sqlite3_stmt *stmt, void *data)
{
    RepoSyncError *repo_sync_error = static_cast<RepoSyncError *>(data);
    repo_sync_error->account_sig = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
    repo_sync_error->repo_id = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
    repo_sync_error->err_id = sqlite3_column_int(stmt, 2);
    return true;
}

int LastSyncError::getRepoSyncError(const QString repo_id)
{
    QString account_sig = seafApplet->accountManager()->currentAccount().getSignature();
    char *sql = sqlite3_mprintf("SELECT * FROM RepoSyncError WHERE accountsig = %Q AND repoid = %Q",
                                toCStr(account_sig), toCStr(repo_id));

    RepoSyncError repo_sync_error;
    int n = sqlite_foreach_selected_row(db_, sql, collectRepoSyncError, &repo_sync_error);
    sqlite3_free(sql);

    if (n == 0) {
        return -1;
    }

    return repo_sync_error.err_id;
}

void LastSyncError::flagRepoSyncError(const QString repo_id, int err_id)
{
    QString account_sig = seafApplet->accountManager()->currentAccount().getSignature();
    char *sql = sqlite3_mprintf("REPLACE INTO RepoSyncError (accountsig, repoid, errid) VALUES (%Q, %Q, %d)",
                                toCStr(account_sig), toCStr(repo_id), err_id);

    sqlite_query_exec(db_, sql);
    sqlite3_free(sql);
}

void LastSyncError::cleanRepoSyncError(const QString repo_id)
{
    QString account_sig = seafApplet->accountManager()->currentAccount().getSignature();
    char *sql = sqlite3_mprintf("DELETE FROM RepoSyncError WHERE accountsig = %Q AND repoid = %Q",
                                toCStr(account_sig), toCStr(repo_id));

    sqlite_query_exec(db_, sql);
    sqlite3_free(sql);
}
