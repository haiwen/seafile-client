#ifndef _SEAF_ACCOUNT_MGR_H
#define _SEAF_ACCOUNT_MGR_H

#include <vector>

#include <QObject>

#include "account.h"

struct sqlite3;
struct sqlite3_stmt;

/**
 * Load/Save seahub accounts
 */
class AccountManager : public QObject {
    Q_OBJECT

public:
    AccountManager();
    ~AccountManager();

    int start();

    int saveAccount(const Account& account);
    int removeAccount(const Account& account);
    const std::vector<Account>& loadAccounts();
    bool accountExists(const QUrl& url, const QString& username);

    bool hasAccount() const { return !accounts_.empty(); }

    Account currentAccount() const { return hasAccount() ? accounts_[0] : Account(); }

    bool setCurrentAccount(const Account& account);

    // accessors
    const std::vector<Account>& accounts() const { return accounts_; }
signals:
    /**
     * Account added/removed/switched.
     */
    void accountsChanged();

private:
    Q_DISABLE_COPY(AccountManager)

    static bool loadAccountsCB(struct sqlite3_stmt *stmt, void *data);

    void updateAccountLastVisited(const Account& account);

    struct sqlite3 *db;
    std::vector<Account> accounts_;
};

#endif  // _SEAF_ACCOUNT_MGR_H
