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
    db = NULL;
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

    QString db_path = QDir(seafApplet->configurator()->seafileDir()).filePath("accounts.db");
    if (sqlite3_open (toCStr(db_path), &db)) {
        errmsg = sqlite3_errmsg (db);
        qDebug("failed to open account database %s: %s",
               toCStr(db_path), errmsg ? errmsg : "no error given");

        seafApplet->errorAndExit(tr("failed to open account database"));
        return -1;
    }

    sql = "CREATE TABLE IF NOT EXISTS Accounts (url VARCHAR(24), "
        "username VARCHAR(15), token VARCHAR(40), lastVisited INTEGER, "
        "PRIMARY KEY(url, username))";
    sqlite_query_exec (db, sql);

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
    int i = 0;
    for (i = 0; i < accounts_.size(); i++) {
        if (accounts_[i].serverUrl == account.serverUrl
            && accounts_[i].username == account.username) {
            accounts_.erase(accounts_.begin() + i);
            break;
        }
    }

    accounts_.insert(accounts_.begin(), account);

    QString url = account.serverUrl.toEncoded().data();
    qint64 timestamp = QDateTime::currentMSecsSinceEpoch();

    QString sql = "REPLACE INTO Accounts VALUES ('%1', '%2', '%3', %4) ";
    sql = sql.arg(url).arg(account.username).arg(account.token).arg(QString::number(timestamp));
    sqlite_query_exec (db, toCStr(sql));

    emit accountsChanged();

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

    emit accountsChanged();

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

bool AccountManager::accountExists(const QUrl& url, const QString& username)
{
    int i, n = accounts_.size();
    for (i = 0; i < n; i++) {
        if (accounts_[i].serverUrl == url && accounts_[i].username == username) {
            return true;
        }
    }

    return false;
}

bool AccountManager::setCurrentAccount(const Account& account)
{
    if (account == currentAccount()) {
        return false;
    }

    // Would emit "accountsChanged" signal
    saveAccount(account);

    return true;
}
