#include <sqlite3.h>
#include <glib.h>
#include <errno.h>
#include <stdio.h>

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
        "username VARCHAR(15), token VARCHAR(40), PRIMARY KEY(url, username))";
    sqlite_query_exec (db, sql);

    g_free (db_path);
    return 0;
}

bool AccountManager::loadAccountsCB(sqlite3_stmt *stmt, void *data)
{
    AccountManager *mgr = (AccountManager *)data;
    const char *url = (const char *)sqlite3_column_text (stmt, 0);
    const char *username = (const char *)sqlite3_column_text (stmt, 1);
    const char *token = (const char *)sqlite3_column_text (stmt, 2);

    Account account = Account(QUrl(QString(url)), QString(username), QString(token));
    mgr->accounts_.push_back(account);
    return true;
}

std::vector<Account> AccountManager::loadAccounts()
{
    const char *sql = "SELECT url, username, token FROM Accounts";
    accounts_.clear();
    sqlite_foreach_selected_row (db, sql, loadAccountsCB, this);
    return accounts_;
}

int AccountManager::saveAccount(const Account& account)
{
    accounts_.push_back(account);

    char sql[4096];
    const QByteArray url = account.serverUrl.toEncoded();
    const QByteArray username = account.username.toUtf8();
    const QByteArray token = account.token.toUtf8();

    snprintf(sql, 4096, "REPLACE INTO Accounts VALUES ('%s', '%s', '%s') ",
             url.data(), username.data(), token.data());
    sqlite_query_exec (db, sql);

    emit accountAdded(account);

    return 0;
}
