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
#include "api/api-error.h"
#include "api/requests.h"
#include "rpc/rpc-client.h"

namespace {
const char *kRepoRelayAddrProperty = "relay-address";

bool compareAccount(const Account& a, const Account& b)
{
    if (!a.isValid()) {
        return false;
    } else if (!b.isValid()) {
        return true;
    } else if (a.lastVisited < b.lastVisited) {
        return false;
    } else if (a.lastVisited > b.lastVisited) {
        return true;
    }

    return true;
}

}

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
        qWarning("failed to open account database %s: %s",
                toCStr(db_path), errmsg ? errmsg : "no error given");

        seafApplet->errorAndExit(tr("failed to open account database"));
        return -1;
    }

    // enabling foreign keys, it must be done manually from each connection
    // and this feature is only supported from sqlite 3.6.19
    sql = "PRAGMA foreign_keys=ON;";
    sqlite_query_exec (db, sql);

    sql = "CREATE TABLE IF NOT EXISTS Accounts (url VARCHAR(24), "
        "username VARCHAR(15), token VARCHAR(40), lastVisited INTEGER, "
        "PRIMARY KEY(url, username))";
    sqlite_query_exec (db, sql);

    loadAccounts();

    connect(this, SIGNAL(accountsChanged()), this, SLOT(onAccountsChanged()));
    return 0;
}

bool AccountManager::loadAccountsCB(sqlite3_stmt *stmt, void *data)
{
    AccountManager *mgr = (AccountManager *)data;
    const char *url = (const char *)sqlite3_column_text (stmt, 0);
    const char *username = (const char *)sqlite3_column_text (stmt, 1);
    const char *token = (const char *)sqlite3_column_text (stmt, 2);
    qint64 atime = (qint64)sqlite3_column_int64 (stmt, 3);

    if (!token) {
        token = "";
    }

    Account account = Account(QUrl(QString(url)), QString(username), QString(token), atime);
    mgr->accounts_.push_back(account);
    return true;
}

const std::vector<Account>& AccountManager::loadAccounts()
{
    const char *sql = "SELECT url, username, token, lastVisited FROM Accounts ";
    accounts_.clear();
    sqlite_foreach_selected_row (db, sql, loadAccountsCB, this);

    std::stable_sort(accounts_.begin(), accounts_.end(), compareAccount);
    return accounts_;
}

int AccountManager::saveAccount(const Account& account)
{
    for (size_t i = 0; i < accounts_.size(); i++) {
        if (accounts_[i] == account) {
            accounts_.erase(accounts_.begin() + i);
            break;
        }
    }
    accounts_.insert(accounts_.begin(), account);
    updateServerInfo(0);

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
    for (size_t i = 0; i < accounts_.size(); i++) {
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

    emit beforeAccountChanged();

    // Would emit "accountsChanged" signal
    saveAccount(account);

    return true;
}

int AccountManager::replaceAccount(const Account& old_account, const Account& new_account)
{
    for (size_t i = 0; i < accounts_.size(); i++) {
        if (accounts_[i] == old_account) {
            accounts_[i] = new_account;
            updateServerInfo(i);
            break;
        }
    }

    QString old_url = old_account.serverUrl.toEncoded().data();
    QString new_url = new_account.serverUrl.toEncoded().data();

    qint64 timestamp = QDateTime::currentMSecsSinceEpoch();

    QString sql =
        "UPDATE Accounts "
        "SET url = '%1', "
        "    username = '%2', "
        "    token = '%3', "
        "    lastVisited = '%4' "
        "WHERE url = '%5' "
        "  AND username = '%2'";

    sql = sql.arg(new_url).arg(new_account.username). \
        arg(new_account.token).arg(QString::number(timestamp)) \
        .arg(old_url);

    sqlite_query_exec (db, toCStr(sql));

    emit accountsChanged();

    return 0;
}

Account AccountManager::getAccountByHostAndUsername(const QString& host,
                                                    const QString& username) const
{
    for (size_t i = 0; i < accounts_.size(); i++) {
        if (accounts_[i].serverUrl.host() == host
            && accounts_[i].username == username) {
            return accounts_[i];
        }
    }

    return Account();
}

Account AccountManager::getAccountBySignature(const QString& account_sig) const
{
    for (size_t i = 0; i < accounts_.size(); i++) {
        if (accounts_[i].getSignature() == account_sig) {
            return accounts_[i];
        }
    }

    return Account();
}

void AccountManager::updateServerInfo()
{
    for (size_t i = 0; i < accounts_.size(); i++)
        updateServerInfo(i);
}

void AccountManager::updateServerInfo(unsigned index)
{
    ServerInfoRequest *request;
    // request is taken owner by Account object
    request = accounts_[index].createServerInfoRequest();
    connect(request, SIGNAL(success(const Account&, const ServerInfo &)),
            this, SLOT(serverInfoSuccess(const Account&, const ServerInfo &)));
    connect(request, SIGNAL(failed(const ApiError&)),
            this, SLOT(serverInfoFailed(const ApiError&)));
    request->send();
}


void AccountManager::serverInfoSuccess(const Account &account, const ServerInfo &info)
{
    for (size_t i = 0; i < accounts_.size(); i++) {
        if (accounts_[i] == account) {
            if (i == 0)
                emit beforeAccountChanged();
            accounts_[i].serverInfo = info;
            if (i == 0)
                emit accountsChanged();
            break;
        }
    }
}

void AccountManager::serverInfoFailed(const ApiError &error)
{
    qWarning("update server info failed %s\n", error.toString().toUtf8().data());
}

bool AccountManager::clearAccountToken(const Account& account)
{
    for (size_t i = 0; i < accounts_.size(); i++) {
        if (accounts_[i].serverUrl.toString() == account.serverUrl.toString()
            && accounts_[i].username == account.username) {
            accounts_.erase(accounts_.begin() + i);
            break;
        }
    }

    Account new_account = account;
    new_account.token = "";

    accounts_.push_back(new_account);

    QString url = account.serverUrl.toEncoded().data();

    QString sql =
        "UPDATE Accounts "
        "SET token = NULL "
        "WHERE url = '%1' "
        "  AND username = '%2'";

    sql = sql.arg(url).arg(account.username);

    sqlite_query_exec (db, toCStr(sql));

    emit accountsChanged();

    return true;
}

Account AccountManager::getAccountByRepo(const QString& repo_id)
{
    SeafileRpcClient *rpc = seafApplet->rpcClient();
    if (!accounts_cache_.contains(repo_id)) {
        QString relay_addr;
        if (rpc->getRepoProperty(repo_id, kRepoRelayAddrProperty, &relay_addr) < 0) {
            return Account();
        }
        const std::vector<Account>& accounts = seafApplet->accountManager()->accounts();
        for (unsigned i = 0; i < accounts.size(); i++) {
            const Account& account = accounts[i];
            if (account.serverUrl.host() == relay_addr) {
                accounts_cache_[repo_id] = account;
                break;
            }
        }
    }
    return accounts_cache_.value(repo_id, Account());
}

void AccountManager::onAccountsChanged()
{
    accounts_cache_.clear();
}
