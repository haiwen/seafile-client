#ifndef _SEAF_ACCOUNT_MGR_H
#define _SEAF_ACCOUNT_MGR_H

#include <vector>

#include <QObject>

#include "account.h"

struct sqlite3;
struct sqlite3_stmt;
class ApiError;

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

    // accessors
    const std::vector<Account>& accounts() const { return accounts_; }
signals:
    /**
     * Account added/removed/switched.
     */
    void beforeAccountChanged();
    void accountsChanged();

private slots:
    void serverInfoSuccess(const Account &account, const ServerInfo &info);
    void serverInfoFailed(const ApiError&);

private:
    Q_DISABLE_COPY(AccountManager)

    void updateServerInfo(unsigned index);
    static bool loadAccountsCB(struct sqlite3_stmt *stmt, void *data);

    void updateAccountLastVisited(const Account& account);

    struct sqlite3 *db;
    std::vector<Account> accounts_;
};

#endif  // _SEAF_ACCOUNT_MGR_H
