#ifndef _SEAF_ACCOUNT_MGR_H
#define _SEAF_ACCOUNT_MGR_H

#include <vector>

#include <QObject>
#include <QUrl>
#include <QString>

#include "account.h"

class AccountManager : public QObject {
    Q_OBJECT
    Q_DISABLE_COPY(AccountManager)
    AccountManager();

public:
    static AccountManager* instance();
    std::vector<Account> loadAccounts();
    int saveAccount(const Account& account);

signals:
    void accountsChanged();

private:
    static AccountManager* singleton_;
    std::vector<Account> accounts_;
};

#endif  // _SEAF_ACCOUNT_MGR_H
