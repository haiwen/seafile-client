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
    std::vector<Account> loadAccounts();
    int saveAccount(const Account& account);
    int start();

private:
    ~AccountManager();
    static bool loadAccountsCB(struct sqlite3_stmt *stmt, void *data);

signals:
    void accountsChanged();

private:
    Q_DISABLE_COPY(AccountManager)

    struct sqlite3 *db;
    std::vector<Account> accounts_;
};

#endif  // _SEAF_ACCOUNT_MGR_H
