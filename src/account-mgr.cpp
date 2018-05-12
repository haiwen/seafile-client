#include <sqlite3.h>
#include <glib.h>
#include <errno.h>
#include <stdio.h>
#include <algorithm>

#include <QDateTime>
#include <QMutexLocker>

#include "account-mgr.h"
#include "configurator.h"
#include "seafile-applet.h"
#include "utils/utils.h"
#include "api/api-error.h"
#include "api/requests.h"
#include "rpc/rpc-client.h"
#include "rpc/local-repo.h"
#include "account-info-service.h"
#include "ui/sso-dialog.h"
#include "settings-mgr.h"

namespace {
const char *kRepoRelayAddrProperty = "relay-address";
const char *kVersionKeyName = "version";
const char *kFeaturesKeyName = "features";
const char *kCustomBrandKeyName = "custom-brand";
const char *kCustomLogoKeyName = "custom-logo";
const char *kTotalStorage = "storage.total";
const char *kUsedStorage = "storage.used";
const char *kNickname = "name";

struct ColumnCheckData {
    QString name;
    bool exists;
};

bool getColumnInfoCallback(sqlite3_stmt *stmt, void *data)
{
    ColumnCheckData *cdata = (ColumnCheckData *)data;
    const char *column_name = (const char *)sqlite3_column_text (stmt, 1);

    if (cdata->name == QString(column_name)) {
        cdata->exists = true;
        return false;
    }

    return true;
}

void addNewColumnToAccountsTable(struct sqlite3* db, const QString& name, const QString& type)
{
    QString sql = "PRAGMA table_info(Accounts);";
    ColumnCheckData cdata;
    cdata.name = name;
    cdata.exists = false;
    sqlite_foreach_selected_row (db, toCStr(sql), getColumnInfoCallback, &cdata);
    if (!cdata.exists) {
        sql = QString("ALTER TABLE Accounts ADD COLUMN %1 %2").arg(name).arg(type);
        if (sqlite_query_exec (db, toCStr(sql)) < 0) {
            qCritical("unable to create column %s\n", toCStr(name));
        }
    }
}

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

QStringList collectSyncedReposForAccount(const Account& account)
{
    std::vector<LocalRepo> repos;
    SeafileRpcClient *rpc = seafApplet->rpcClient();
    rpc->listLocalRepos(&repos);
    QStringList repo_ids;
    for (size_t i = 0; i < repos.size(); i++) {
        LocalRepo repo = repos[i];
        QString repo_server_url;
        if (rpc->getRepoProperty(repo.id, "server-url", &repo_server_url) < 0) {
            continue;
        }
        if (QUrl(repo_server_url).host() != account.serverUrl.host()) {
            continue;
        }
        QString token;
        if (rpc->getRepoProperty(repo.id, "token", &token) < 0 || token.isEmpty()) {
            repo_ids.append(repo.id);
        }
    }

    return repo_ids;
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

    addNewColumnToAccountsTable(db, "isShibboleth", "INTEGER");
    addNewColumnToAccountsTable(db, "AutomaticLogin", "INTEGER default 1");
    addNewColumnToAccountsTable(db, "s2fa_token", "TEXT");

    // ServerInfo table is used to store any (key, value) information for an
    // account.
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
    connect(qApp, SIGNAL(aboutToQuit()), this, SLOT(onAboutToQuit()));
    connect(this, SIGNAL(accountRequireRelogin(const Account&)),
            this, SLOT(reloginAccount(const Account &)));
    return 0;
}

void AccountManager::onAboutToQuit()
{
    logoutDeviceNonautoLogin();
}

bool AccountManager::loadAccountsCB(sqlite3_stmt *stmt, void *data)
{
    UserData *userdata = static_cast<UserData*>(data);
    const char *url = (const char *)sqlite3_column_text (stmt, 0);
    const char *username = (const char *)sqlite3_column_text (stmt, 1);
    const char *token = (const char *)sqlite3_column_text (stmt, 2);
    qint64 atime = (qint64)sqlite3_column_int64 (stmt, 3);
    int isShibboleth = sqlite3_column_int (stmt, 4);
    int isAutomaticLogin = sqlite3_column_int (stmt, 5);
    const char *s2fa_token = (const char *)sqlite3_column_text (stmt,6);

    if (!token) {
        token = "";
    }

    if (!s2fa_token) {
        s2fa_token = "";
    }

    Account account = Account(QUrl(QString(url)), QString(username),
                              QString(token), atime, isShibboleth != 0,
                              isAutomaticLogin != 0, QString(s2fa_token));
    char* zql = sqlite3_mprintf("SELECT key, value FROM ServerInfo WHERE url = %Q AND username = %Q", url, username);
    sqlite_foreach_selected_row (userdata->db, zql, loadServerInfoCB, &account);
    sqlite3_free(zql);

    userdata->accounts->push_back(account);
    return true;
}

bool AccountManager::loadServerInfoCB(sqlite3_stmt *stmt, void *data)
{
    Account *account = static_cast<Account*>(data);
    const char *key = (const char *)sqlite3_column_text (stmt, 0);
    const char *value = (const char *)sqlite3_column_text (stmt, 1);
    QString key_string = key;
    QString value_string = value;
    if (key_string == kVersionKeyName) {
        account->serverInfo.parseVersionFromString(value_string);
    } else if (key_string == kFeaturesKeyName) {
        account->serverInfo.parseFeatureFromStrings(value_string.split(","));
    } else if (key_string == kCustomBrandKeyName) {
        account->serverInfo.customBrand = value_string;
    } else if (key_string == kCustomLogoKeyName) {
        account->serverInfo.customLogo = value_string;
    } else if (key_string == kTotalStorage) {
        account->accountInfo.totalStorage = value_string.toLongLong();
    } else if (key_string == kUsedStorage) {
        account->accountInfo.usedStorage = value_string.toLongLong();
    } else if (key_string == kNickname) {
        account->accountInfo.name = value_string;
    }
    return true;
}

const std::vector<Account>& AccountManager::loadAccounts()
{
    const char *sql = "SELECT url, username, token, lastVisited, isShibboleth, AutomaticLogin, s2fa_token "
                      "FROM Accounts ORDER BY lastVisited DESC";
    accounts_.clear();
    UserData userdata;
    userdata.accounts = &accounts_;
    userdata.db = db;
    sqlite_foreach_selected_row (db, sql, loadAccountsCB, &userdata);

    std::stable_sort(accounts_.begin(), accounts_.end(), compareAccount);
    return accounts_;
}

int AccountManager::saveAccount(const Account& account)
{
    Account new_account = account;
    bool account_exist = false;
    {
        QMutexLocker lock(&accounts_mutex_);
        for (size_t i = 0; i < accounts_.size(); i++) {
            if (accounts_[i] == account) {
                accounts_.erase(accounts_.begin() + i);
                account_exist = true;
                break;
            }
        }
        accounts_.insert(accounts_.begin(), new_account);
    }
    updateServerInfo(0);

    qint64 timestamp = QDateTime::currentMSecsSinceEpoch();

    char *zql;
    if (account_exist) {
        zql = sqlite3_mprintf(
            "UPDATE Accounts SET token = %Q, lastVisited = %Q, isShibboleth = %Q, AutomaticLogin = %Q, s2fa_token = %Q"
            "WHERE url = %Q AND username = %Q",
            // token
            new_account.token.toUtf8().data(),
            // lastVisited
            QString::number(timestamp).toUtf8().data(),
            // isShibboleth
            QString::number(new_account.isShibboleth).toUtf8().data(),
            // isAutomaticLogin
            QString::number(new_account.isAutomaticLogin).toUtf8().data(),
            //s2fa_token
            new_account.s2fa_token.toUtf8().data(),
            // url
            new_account.serverUrl.toEncoded().data(),
            // username
            new_account.username.toUtf8().data());
    } else {
        zql = sqlite3_mprintf(
            "INSERT INTO Accounts(url, username, token, lastVisited, isShibboleth, AutomaticLogin, s2fa_token) "
            "VALUES (%Q, %Q, %Q, %Q, %Q, %Q, %Q) ",
            // url
            new_account.serverUrl.toEncoded().data(),
            // username
            new_account.username.toUtf8().data(),
            // token
            new_account.token.toUtf8().data(),
            // lastVisited
            QString::number(timestamp).toUtf8().data(),
            // isShibboleth
            QString::number(new_account.isShibboleth).toUtf8().data(),
            // isAutomaticLogin
            QString::number(new_account.isAutomaticLogin).toUtf8().data(),
            //s2fa_token
            new_account.s2fa_token.toUtf8().data());
    }
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

    bool need_switch_account = currentAccount() == account;

    {
        QMutexLocker lock(&accounts_mutex_);
        accounts_.erase(
            std::remove(accounts_.begin(), accounts_.end(), account),
            accounts_.end());
    }

    if (need_switch_account) {
        if (!accounts_.empty()) {
            validateAndUseAccount(accounts_[0]);
        } else {
            SSODialog login_dialog;
            login_dialog.exec();
        }
    }

    emit accountsChanged();

    return 0;
}

void AccountManager::logoutDevice(const Account& account)
{
    clearSyncToken(account);
    clearAccountToken(account);
}

void AccountManager::logoutDeviceNonautoLogin()
{
    QMutexLocker lock(&accounts_mutex_);
    for (const Account& account : accounts_) {
        if (account.isAutomaticLogin) {
            continue;
        }
        clearSyncToken(account);
        clearAccountToken(account);
    }
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

bool AccountManager::validateAndUseAccount(const Account& account)
{
    if (!account.isAutomaticLogin) {
        clearAccountToken(account);
        return reloginAccount(account);
    }
    else if (!account.isValid()) {
        return reloginAccount(account);
    }
    else {
        return setCurrentAccount(account);
    }
}

bool AccountManager::setCurrentAccount(const Account& account)
{
    Q_ASSERT(account.isValid());

    if (account == currentAccount()) {
        return false;
    }

    emit beforeAccountSwitched();

    // Would emit "accountsChanged" signal
    saveAccount(account);

    AccountInfoService::instance()->refresh();

    return true;
}

int AccountManager::replaceAccount(const Account& old_account, const Account& new_account)
{
    {
        QMutexLocker lock(&accounts_mutex_);
        for (size_t i = 0; i < accounts_.size(); i++) {
            if (accounts_[i] == old_account) {
                // TODO copy new_account and old_account before this operation
                // we might have invalid old_account or new_account after it
                accounts_[i] = new_account;
                updateServerInfo(i);
                break;
            }
        }
    }

    qint64 timestamp = QDateTime::currentMSecsSinceEpoch();

    char *zql = sqlite3_mprintf(
        "UPDATE Accounts "
        "SET url = %Q, "
        "    username = %Q, "
        "    token = %Q, "
        "    lastVisited = %Q, "
        "    isShibboleth = %Q, "
        "    AutomaticLogin = %Q "
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
        // isShibboleth
        QString::number(new_account.isShibboleth).toUtf8().data(),
        // isAutomaticLogin
        QString::number(new_account.isAutomaticLogin).toUtf8().data(),
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

void AccountManager::updateAccountInfo(const Account& account,
                                       const AccountInfo& info)
{
    setServerInfoKeyValue(db, account, kTotalStorage,
                          QString::number(info.totalStorage));
    setServerInfoKeyValue(db, account, kUsedStorage,
                          QString::number(info.usedStorage));
    setServerInfoKeyValue(db, account, kNickname,
                          info.name);

    for (size_t i = 0; i < accounts_.size(); i++) {
        if (accounts_[i] == account) {
            accounts_[i].accountInfo = info;
            emit accountInfoUpdated(accounts_[i]);
            break;
        }
    }
}


void AccountManager::serverInfoSuccess(const Account &_account, const ServerInfo &info)
{
    Account account = _account;
    account.serverInfo = info;

    setServerInfoKeyValue(db, account, kVersionKeyName, info.getVersionString());
    setServerInfoKeyValue(db, account, kFeaturesKeyName, info.getFeatureStrings().join(","));
    setServerInfoKeyValue(db, account, kCustomLogoKeyName, info.customLogo);
    setServerInfoKeyValue(db, account, kCustomBrandKeyName, info.customBrand);

    bool changed = _account.serverInfo != info;
    if (!changed)
        return;

    QUrl url(account.serverUrl);
    url.setPath("/");
    seafApplet->rpcClient()->setServerProperty(
        url.toString(), "is_pro", account.isPro() ? "true" : "false");

    for (size_t i = 0; i < accounts_.size(); i++) {
        if (accounts_[i] == account) {
            if (i == 0)
                emit beforeAccountSwitched();
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
        if (accounts_[i] == account) {
            accounts_[i].token = "";
            break;
        }
    }

    char *zql = sqlite3_mprintf(
        "UPDATE Accounts "
        "SET token = NULL "
        "WHERE url = %Q "
        "  AND username = %Q",
        // url
        account.serverUrl.toEncoded().data(),
        // username
        account.username.toUtf8().data()
        );
    sqlite_query_exec(db, zql);
    sqlite3_free(zql);

    emit accountsChanged();

    return true;
}

bool AccountManager::clearSyncToken(const Account& account)
{
    QString error;
    if (seafApplet->rpcClient()->removeSyncTokensByAccount(account.serverUrl.host(),
                                                           account.username,
                                                           &error)  < 0) {
        seafApplet->warningBox(
            tr("Failed to remove local repos sync token: %1").arg(error));
        return false;
    } else {
        return true;
    }
}

void AccountManager::removeNonautoLoginSyncTokens()
{
    QMutexLocker lock(&accounts_mutex_);
    for (const Account& account : accounts_) {
        if (account.isAutomaticLogin) {
            continue;
        }

        clearSyncToken(account);
    }
    return;
}

Account AccountManager::getAccountByRepo(const QString& repo_id, SeafileRpcClient *rpc)
{
    std::vector<Account> accounts;
    {
        QMutexLocker lock(&accounts_mutex_);
        accounts = accounts_;
    }

    QMutexLocker cache_lock(&accounts_cache_mutex_);

    if (!accounts_cache_.contains(repo_id)) {
        QString relay_addr;
        if (rpc->getRepoProperty(repo_id, kRepoRelayAddrProperty, &relay_addr) < 0) {
            return Account();
        }

        for (size_t i = 0; i < accounts.size(); i++) {
            const Account& account = accounts[i];
            if (account.serverUrl.host() == relay_addr) {
                accounts_cache_[repo_id] = account;
                break;
            }
        }
    }
    return accounts_cache_.value(repo_id, Account());
}

Account AccountManager::getAccountByRepo(const QString& repo_id)
{
    return getAccountByRepo(repo_id, seafApplet->rpcClient());
}

void AccountManager::onAccountsChanged()
{
    QMutexLocker cache_lock(&accounts_cache_mutex_);
    accounts_cache_.clear();
}

void AccountManager::invalidateCurrentLogin()
{
    // make sure we have accounts there
    if (!hasAccount())
        return;
    const Account &account = accounts_.front();
    // if the token is already invalidated, ignore
    if (account.token.isEmpty())
        return;

    emit accountAboutToRelogin(account);

    clearSyncToken(account);
    clearAccountToken(account);
    seafApplet->warningBox(tr("Authorization expired, please re-login"));

    emit accountRequireRelogin(account);
}

bool AccountManager::reloginAccount(const Account &account_in)
{
    qWarning("Relogin to account %s", account_in.username.toUtf8().data());
    bool accepted;

    // Make a copy of the account arugment because it may be released after the
    // login succeeded.
    //
    // See: https://github.com/haiwen/seafile-client/blob/v6.1.3/src/account-mgr.cpp#L219
    // See: https://gist.github.com/lins05/f952356ba8733d5aa19b54a6db19f69a
    const Account account(account_in);

    SSODialog dialog;
    accepted = dialog.exec() == QDialog::Accepted;

    if (accepted) {
        getSyncedReposToken(account);
    }

    return accepted;
}

void AccountManager::getSyncedReposToken(const Account& account)
{
    QStringList repo_ids = collectSyncedReposForAccount(account);
    if (repo_ids.empty()) {
        return;
    }

    /* old account object don't contains the new token */
    QString host = account.serverUrl.host();
    QString username = account.username;
    Account new_account = getAccountByHostAndUsername(host, username);
    if (!new_account.isValid())
        return;

    // For debugging lots of repos problem.
    // TODO: Comment this out before committing!!
    //
    // int targetNumberForDebug = 300;
    // while (repo_ids.size() < targetNumberForDebug) {
    //     repo_ids.append(repo_ids);
    // }
    // repo_ids = repo_ids.mid(0, 300);
    // printf ("repo_ids.size() = %d\n", repo_ids.size());

    GetRepoTokensRequest *req = new GetRepoTokensRequest(
        new_account, repo_ids);

    connect(req, SIGNAL(success()),
            this, SLOT(onGetRepoTokensSuccess()));
    connect(req, SIGNAL(failed(const ApiError&)),
            this, SLOT(onGetRepoTokensFailed(const ApiError&)));
    req->send();
}

void AccountManager::onGetRepoTokensSuccess()
{
    GetRepoTokensRequest *req = (GetRepoTokensRequest *)(sender());
    foreach (const QString& repo_id, req->repoTokens().keys()) {
        seafApplet->rpcClient()->setRepoToken(
            repo_id, req->repoTokens().value(repo_id));
    }
    req->deleteLater();
}

void AccountManager::onGetRepoTokensFailed(const ApiError& error)
{
    GetRepoTokensRequest *req = (GetRepoTokensRequest *)QObject::sender();
    req->deleteLater();
    seafApplet->warningBox(
        tr("Failed to get repo sync information from server: %1").arg(error.toString()));
}
