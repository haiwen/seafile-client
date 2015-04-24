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

struct UserData {
    std::vector<Account> *accounts;
    struct sqlite3 *db;
};

inline void setServerInfoKeyValue(struct sqlite3 *db, const Account &account, const QString& key, const QString &value)
{
    char *zql = sqlite3_mprintf(
        "REPLACE INTO ServerInfo(url, username, key, value) VALUES (%Q, %Q, %Q, %Q)",
        account.serverUrl.toEncoded().data(), account.username.toUtf8().data(),
        key.toUtf8().data(), value.toUtf8().data());
    sqlite_query_exec(db, zql);
    sqlite3_free(zql);
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
        qCritical("failed to open account database %s: %s",
                toCStr(db_path), errmsg ? errmsg : "no error given");

        seafApplet->errorAndExit(tr("failed to open account database"));
        return -1;
    }

    // enabling foreign keys, it must be done manually from each connection
    // and this feature is only supported from sqlite 3.6.19
    sql = "PRAGMA foreign_keys=ON;";
    if (sqlite_query_exec (db, sql) < 0) {
        qCritical("sqlite version is too low to support foreign key feature\n");
        sqlite3_close(db);
        db = NULL;
        return -1;
    }

    sql = "CREATE TABLE IF NOT EXISTS Accounts (url VARCHAR(24), "
        "username VARCHAR(15), token VARCHAR(40), lastVisited INTEGER, "
        "PRIMARY KEY(url, username))";
    if (sqlite_query_exec (db, sql) < 0) {
        qCritical("failed to create accounts table\n");
        sqlite3_close(db);
        db = NULL;
        return -1;
    }

    // create ServerInfo table
    sql = "CREATE TABLE IF NOT EXISTS ServerInfo ("
        "key TEXT NOT NULL, value TEXT, "
        "url VARCHAR(24), username VARCHAR(15), "
        "PRIMARY KEY(url, username, key), "
        "FOREIGN KEY(url, username) REFERENCES Accounts(url, username) "
        "ON DELETE CASCADE ON UPDATE CASCADE )";
    if (sqlite_query_exec (db, sql) < 0) {
        qCritical("failed to create server_info table\n");
        sqlite3_close(db);
        db = NULL;
        return -1;
    }

    loadAccounts();

    connect(this, SIGNAL(accountsChanged()), this, SLOT(onAccountsChanged()));
    return 0;
}

bool AccountManager::loadAccountsCB(sqlite3_stmt *stmt, void *data)
{
    UserData *userdata = static_cast<UserData*>(data);
    const char *url = (const char *)sqlite3_column_text (stmt, 0);
    const char *username = (const char *)sqlite3_column_text (stmt, 1);
    const char *token = (const char *)sqlite3_column_text (stmt, 2);
    qint64 atime = (qint64)sqlite3_column_int64 (stmt, 3);

    if (!token) {
        token = "";
    }

    Account account = Account(QUrl(QString(url)), QString(username), QString(token), atime);
    char* zql = sqlite3_mprintf("SELECT key, value FROM ServerInfo WHERE url = %Q AND username = %Q", url, username);
    sqlite_foreach_selected_row (userdata->db, zql, loadServerInfoCB, &account.serverInfo);
    sqlite3_free(zql);

    userdata->accounts->push_back(account);
    return true;
}

bool AccountManager::loadServerInfoCB(sqlite3_stmt *stmt, void *data)
{
    ServerInfo *info = static_cast<ServerInfo*>(data);
    const char *key = (const char *)sqlite3_column_text (stmt, 0);
    const char *value = (const char *)sqlite3_column_text (stmt, 1);
    QString key_string = key;
    QString value_string = value;
    if (key_string == "version") {
        info->parseVersionFromString(value_string);
    } else {
        info->parseFeatureFromString(key_string, value_string.toLower() == "true");
    }
    return true;
}

const std::vector<Account>& AccountManager::loadAccounts()
{
    const char *sql = "SELECT url, username, token, lastVisited FROM Accounts ";
    accounts_.clear();
    UserData userdata = { .accounts = &accounts_, .db = db };
    sqlite_foreach_selected_row (db, sql, loadAccountsCB, &userdata);

    std::stable_sort(accounts_.begin(), accounts_.end(), compareAccount);
    return accounts_;
}

int AccountManager::saveAccount(const Account& account)
{
    Account new_account = account;
    for (size_t i = 0; i < accounts_.size(); i++) {
        if (accounts_[i] == account) {
            accounts_.erase(accounts_.begin() + i);
            break;
        }
    }
    accounts_.insert(accounts_.begin(), new_account);
    updateServerInfo(0);

    qint64 timestamp = QDateTime::currentMSecsSinceEpoch();

    char *zql = sqlite3_mprintf(
        "REPLACE INTO Accounts(url, username, token, lastVisited) VALUES (%Q, %Q, %Q, %Q) ",
        // url
        new_account.serverUrl.toEncoded().data(),
        // username
        new_account.username.toUtf8().data(),
        // token
        new_account.token.toUtf8().data(),
        // lastVisited
        QString::number(timestamp).toUtf8().data());
    sqlite_query_exec(db, zql);
    sqlite3_free(zql);

    emit accountsChanged();

    return 0;
}

int AccountManager::removeAccount(const Account& account)
{
    char *zql = sqlite3_mprintf(
        "DELETE FROM Accounts WHERE url = %Q AND username = %Q",
        // url
        account.serverUrl.toEncoded().data(),
        // username
        account.username.toUtf8().data());
    sqlite_query_exec(db, zql);
    sqlite3_free(zql);

    accounts_.erase(std::remove(accounts_.begin(), accounts_.end(), account),
                    accounts_.end());

    emit accountsChanged();

    return 0;
}

void AccountManager::updateAccountLastVisited(const Account& account)
{
    char *zql = sqlite3_mprintf(
        "UPDATE Accounts SET lastVisited = %Q "
        "WHERE url = %Q AND username = %Q",
        // lastVisted
        QString::number(QDateTime::currentMSecsSinceEpoch()).toUtf8().data(),
        // url
        account.serverUrl.toEncoded().data(),
        // username
        account.username.toUtf8().data());
    sqlite_query_exec(db, zql);
    sqlite3_free(zql);
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
            // TODO copy new_account and old_account before this operation
            // we might have invalid old_account or new_account after it
            accounts_[i] = new_account;
            updateServerInfo(i);
            break;
        }
    }

    qint64 timestamp = QDateTime::currentMSecsSinceEpoch();

    char *zql = sqlite3_mprintf(
        "UPDATE Accounts "
        "SET url = %Q, "
        "    username = %Q, "
        "    token = %Q, "
        "    lastVisited = %Q "
        "WHERE url = %Q "
        "  AND username = %Q",
        // new_url
        new_account.serverUrl.toEncoded().data(),
        // username
        new_account.username.toUtf8().data(),
        // token
        new_account.token.toUtf8().data(),
        // lastvisited
        QString::number(timestamp).toUtf8().data(),
        // old_url
        old_account.serverUrl.toEncoded().data(),
        // username
        new_account.username.toUtf8().data()
        );

    sqlite_query_exec(db, zql);
    sqlite3_free(zql);

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
    const QStringList features = info.getFeatureStrings();
    setServerInfoKeyValue(db, account, "version", info.getVersionString());
    Q_FOREACH(const QString& feature, features)
    {
        setServerInfoKeyValue(db, account, feature, "true");
    }

    bool changed = account.serverInfo != info;
    if (!changed)
        return;

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
    Account new_account = account;
    new_account.token = "";

    for (size_t i = 0; i < accounts_.size(); i++) {
        if (accounts_[i].serverUrl.toString() == account.serverUrl.toString()
            && accounts_[i].username == account.username) {
            accounts_.erase(accounts_.begin() + i);
            break;
        }
    }

    accounts_.push_back(new_account);

    char *zql = sqlite3_mprintf(
        "UPDATE Accounts "
        "SET token = NULL "
        "WHERE url = %Q "
        "  AND username = %Q",
        // url
        new_account.serverUrl.toEncoded().data(),
        // username
        new_account.username.toUtf8().data()
        );
    sqlite_query_exec(db, zql);
    sqlite3_free(zql);

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

void AccountManager::invalidateCurrentLogin()
{
    // make sure we have accounts there
    if (!hasAccount())
        return;
    const Account &account = accounts().front();
    // if the token is already invalidated, ignore
    if (account.token.isEmpty())
        return;
    clearAccountToken(account);
    seafApplet->warningBox(tr("Authorization expired, please re-login"));

    // we have the expire account at the last position
    emit accountRequireRelogin(accounts().back());
}
