#include <sqlite3.h>
#include <glib.h>
#include <errno.h>
#include <stdio.h>
#include <algorithm>

#include <QDateTime>

#include "account-mgr.h"
#include "configurator.h"
#include "seafile-applet.h"
#include "utils/utils.h"

AccountManager::AccountManager()
{
}

AccountManager::~AccountManager()
{
    if (db)
        sqlite3_close(db);
}

int AccountManager::start()
{
    const char *errmsg;
    const char *sql;
    char *db_path;
    const QString config_dir = seafApplet->configurator()->ccnetDir();
    const QByteArray path = config_dir.toUtf8();
    char *db_dir = g_build_filename (path.data(), "misc", NULL);

    if (checkdir_with_mkdir(db_dir) < 0) {
        qDebug ("Cannot open db dir %s: %s\n", db_dir, strerror(errno));
        g_free (db_dir);
        return -1;
    }
    g_free (db_dir);
    db_path = g_build_filename(path.data(), "misc", "accounts.db", NULL);
    if (sqlite3_open (db_path, &db)) {
        errmsg = sqlite3_errmsg (db);
        qDebug("Couldn't open database:'%s', %s\n",
                   db_path, errmsg ? errmsg : "no error given");
        db = NULL;
        return -1;
    }

    sql = "CREATE TABLE IF NOT EXISTS Accounts (url VARCHAR(24), "
        "username VARCHAR(15), token VARCHAR(40), lastVisited INTEGER, "
        "PRIMARY KEY(url, username))";
    sqlite_query_exec (db, sql);

    g_free (db_path);

    loadAccounts();
    return 0;
}

bool AccountManager::loadAccountsCB(sqlite3_stmt *stmt, void *data)
{
    AccountManager *mgr = (AccountManager *)data;
    const char *url = (const char *)sqlite3_column_text (stmt, 0);
    const char *username = (const char *)sqlite3_column_text (stmt, 1);
    const char *token = (const char *)sqlite3_column_text (stmt, 2);
    qint64 atime = (qint64)sqlite3_column_int64 (stmt, 3);

    Account account = Account(QUrl(QString(url)), QString(username), QString(token), atime);
    mgr->accounts_.push_back(account);
    return true;
}

const std::vector<Account>& AccountManager::loadAccounts()
{
    const char *sql = "SELECT url, username, token, lastVisited FROM Accounts "
        "ORDER BY lastVisited DESC";
    accounts_.clear();
    sqlite_foreach_selected_row (db, sql, loadAccountsCB, this);
    return accounts_;
}

int AccountManager::saveAccount(const Account& account)
{
    accounts_.push_back(account);

    QString url = account.serverUrl.toEncoded().data();
    qint64 timestamp = QDateTime::currentMSecsSinceEpoch();

    QString sql = "REPLACE INTO Accounts VALUES ('%1', '%2', '%3', %4) ";
    sql = sql.arg(url).arg(account.username).arg(account.token).arg(QString::number(timestamp));
    sqlite_query_exec (db, toCStr(sql));

    emit accountAdded(account);

    return 0;
}

int AccountManager::removeAccount(const Account& account)
{
    QString url = account.serverUrl.toEncoded().data();

    QString sql = "DELETE FROM Accounts WHERE url = '%1' AND username = '%2'";
    sql = sql.arg(url).arg(account.username);
    sqlite_query_exec (db, toCStr(sql));

    accounts_.erase(std::remove(accounts_.begin(), accounts_.end(), account),
                    accounts_.end());

    emit accountRemoved(account);

    return 0;
}

void AccountManager::updateAccountLastVisited(const Account& account)
{
    const char *url = account.serverUrl.toEncoded().data();

    QString sql = "UPDATE Accounts SET lastVisited = %1 "
        "WHERE username = '%2' AND url = '%3'";

    qint64 timestamp = QDateTime::currentMSecsSinceEpoch();
    sql = sql.arg(QString::number(timestamp)).arg(account.username).arg(url);
    sqlite_query_exec (db, toCStr(sql));
}
