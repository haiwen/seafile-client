#ifndef ACCOUNT_ITEM_H
#define ACCOUNT_ITEM_H

#include <QWidget>
#include "ui_account-item.h"

#include "account.h"

class AccountItem : public QWidget,
                    public Ui::AccountItem
{
    Q_OBJECT
public:
    AccountItem(const Account& account, QWidget *parent=0);
    void setAccount(const Account& account);
    const Account& account() const { return account_; }

private slots:
    void getAccountRepos();

private:
    Account account_;
};


#endif // ACCOUNT_ITEM_H
