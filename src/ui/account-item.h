#ifndef ACCOUNT_ITEM_H
#define ACCOUNT_ITEM_H

#include <QWidget>
#include "ui_account-item.h"

class Account;

class AccountItem : public QWidget,
                    public Ui::AccountItem
{
    Q_OBJECT
public:
    AccountItem(QWidget *parent, const Account& account);
    void setAccount(const Account& account);

private:    
    Account *account_;
};


#endif // ACCOUNT_ITEM_H
