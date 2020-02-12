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


class AccountManager : public QObject {
    Q_OBJECT

public:
    AccountManager();
    ~AccountManager();

    int start();

    // Load the accounts from local db when client starts.
    const std::vector<Account>& loadAccounts();

    /**
     * Account operations
     */

    // Add a new account. Used in login dialogs or read the
    // preconfigured account by system admin.
    void saveAccount(const Account& account);

    // Remove the account. Used when user removes an account from the
    // account menu.
    int removeAccount(const Account& account);

    // Switch to the given account. Used when user tries to switch to
    // another account from the accounts menu.
    bool setCurrentAccount(const Account& account);

    // Update the account details. Currently it's only used to update
    // the server address in AccountSettingsDialog.
    // TODO: replace this with a more restricted `updateAccountURL` interface.
    int replaceAccount(const Account& old_account,
                       const Account& new_account);

    // Use the account if it's valid, otherwise require a re-login.
    bool validateAndUseAccount(const Account& account);

    // Called when API returns 401 and we need to re-login current
    // account.
    void invalidateCurrentLogin();

    // Update AccountInfo (e.g. nick name, quota etc.) for the given
    // account.
    void updateAccountInfo(const Account& account,
                           const AccountInfo& info);

    // Trigger server info refresh for all accounts when client
    // starts.
    void updateServerInfoForAllAccounts();

    /**
     * Logout
     */

    // Called When the user chooses to log out current account.
    void logoutDevice(const Account& account);
    // Before client exits, remove sync tokens for all accounts that
    // doesn't have auot-login set.
    void removeNonautoLoginSyncTokens();

    /**
     * Accessors
     */

    const std::vector<Account>& accounts() const;

    const Account currentAccount() const;

    bool hasAccount() const;

    bool accountExists(const QUrl& url, const QString& username) const;

    Account getAccountByHostAndUsername(const QString& host,
                                        const QString& username) const;

    Account getAccountBySignature(const QString& account_sig) const;

    Account getAccountByRepo(const QString& repo_id);

    Account getAccountByRepo(const QString& repo_id, SeafileRpcClient *rpc);

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

    void updateAccountServerInfo(const Account& account);
    static bool loadAccountsCB(struct sqlite3_stmt *stmt, void *data);
    static bool loadServerInfoCB(struct sqlite3_stmt *stmt, void *data);

    void updateAccountLastVisited(const Account& account);
    void getSyncedReposToken(const Account& account);
    void sendGetRepoTokensRequet(const Account& account, const QStringList& repo_ids, int max_retries);

    void logoutDeviceNonautoLogin();
    bool clearAccountToken(const Account& account);
    bool clearSyncToken(const Account& account);

    QHash<QString, Account> accounts_cache_;

    struct sqlite3 *db;
    std::vector<Account> accounts_;

    QMutex accounts_mutex_;
    QMutex accounts_cache_mutex_;
};

#endif  // _SEAF_ACCOUNT_MGR_H
