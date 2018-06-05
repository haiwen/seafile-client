#ifndef _SEAF_ACCOUNT_MGR_H
#define _SEAF_ACCOUNT_MGR_H

#include <vector>

#include <QObject>
#include <QHash>
#include <QMutex>

#include "account.h"

struct sqlite3;
struct sqlite3_stmt;
class ApiError;
class SeafileRpcClient;

/**
 * Load/Save seahub accounts
 */
class AccountManager : public QObject {
    Q_OBJECT

public:
    AccountManager();
    ~AccountManager();

    int start();
    void updateServerInfo();

    int saveAccount(const Account& account);
    int removeAccount(const Account& account);

    void logoutDevice(const Account& account);
    void logoutDeviceNonautoLogin();

    bool clearAccountToken(const Account& account);
    bool clearSyncToken(const Account& account);
    void removeNonautoLoginSyncTokens();

    const std::vector<Account>& loadAccounts();
    bool accountExists(const QUrl& url, const QString& username);

    bool hasAccount() const { return !accounts_.empty(); }

    Account currentAccount() const { return hasAccount() ? accounts_[0] : Account(); }

    bool setCurrentAccount(const Account& account);

    int replaceAccount(const Account& old_account,
                       const Account& new_account);

    Account getAccountByHostAndUsername(const QString& host,
                                        const QString& username) const;

    Account getAccountBySignature(const QString& account_sig) const;

    /// \brief find the Account By Repo ID
    /// return an invalid Account if failed
    Account getAccountByRepo(const QString& repo_id);

    // Also used by extension handler
    Account getAccountByRepo(const QString& repo_id, SeafileRpcClient *rpc);

    void updateAccountInfo(const Account& account, const AccountInfo& info);

    bool validateAndUseAccount(const Account& account);

    // accessors
    const std::vector<Account>& accounts() const { return accounts_; }

    // invalidate current login and emit a re-login signal
    void invalidateCurrentLogin();

    void getSyncedReposToken(const Account& account);

signals:
    /**
     * Account added/removed/switched.
     */
    void beforeAccountSwitched();
    void accountsChanged();
    void accountAboutToRelogin(const Account& account);
    void accountRequireRelogin(const Account& account);

    void requireAddAccount();
    void accountInfoUpdated(const Account& account);

public slots:
    bool reloginAccount(const Account &account);

private slots:
    void serverInfoSuccess(const Account &account, const ServerInfo &info);
    void serverInfoFailed(const ApiError&);

    void onAccountsChanged();

    void onAboutToQuit();

    void onGetRepoTokensSuccess();
    void onGetRepoTokensFailed(const ApiError& error);

private:
    Q_DISABLE_COPY(AccountManager)

    void updateServerInfo(unsigned index);
    static bool loadAccountsCB(struct sqlite3_stmt *stmt, void *data);
    static bool loadServerInfoCB(struct sqlite3_stmt *stmt, void *data);

    void updateAccountLastVisited(const Account& account);

    QHash<QString, Account> accounts_cache_;

    struct sqlite3 *db;
    std::vector<Account> accounts_;

    QMutex accounts_mutex_;
    QMutex accounts_cache_mutex_;
};

#endif  // _SEAF_ACCOUNT_MGR_H
