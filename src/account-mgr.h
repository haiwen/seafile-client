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
    int start();

    int saveAccount(const Account& account);
    int removeAccount(const Account& account);
    const std::vector<Account>& loadAccounts();
    void updateAccountLastVisited(const Account& account);

    bool hasAccount(const QUrl& url, const QString& username);

    // accessors
    const std::vector<Account>& accounts() const { return accounts_; }

signals:
    void accountAdded(const Account&);
    void accountRemoved(const Account&);

private:
    ~AccountManager();
    static bool loadAccountsCB(struct sqlite3_stmt *stmt, void *data);

private:
    Q_DISABLE_COPY(AccountManager)

    struct sqlite3 *db;
    std::vector<Account> accounts_;
};

#endif  // _SEAF_ACCOUNT_MGR_H
