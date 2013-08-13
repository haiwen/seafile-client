#ifndef _SEAF_ACCOUNT_MGR_H
#define _SEAF_ACCOUNT_MGR_H

#include <vector>

#include <QObject>

#include "account.h"

/**
 * Load/Save seahub accounts
 */
class AccountManager : public QObject {
    Q_OBJECT

public:
    AccountManager();
    std::vector<Account> loadAccounts();
    int saveAccount(const Account& account);

signals:
    void accountsChanged();

private:
    Q_DISABLE_COPY(AccountManager)

    std::vector<Account> accounts_;
};

#endif  // _SEAF_ACCOUNT_MGR_H
