#include <cstdio>
#include <QtGui>

#include "account-view.h"
#include "account-mgr.h"
#include "login-dialog.h"
#include "account-item.h"

AccountView::AccountView(QWidget *parent) : QWidget(parent)
{
    setupUi(this);
    connect(mAddAccountBtn, SIGNAL(clicked()),
            this, SLOT(showAddAccountDialog()));

    mAccountList->setLayout(new QVBoxLayout);

    refreshAccounts();
}

void AccountView::showAddAccountDialog()
{
    LoginDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        refreshAccounts();
    }
}

bool AccountView::hasAccount(const Account& account)
{
    for (std::vector<AccountItem*>::iterator item_iter = accounts_list_.begin();
         item_iter != accounts_list_.end(); item_iter++) {

        AccountItem *item = *item_iter;
        if (item->account() == account) {
            return true;
        }
    }

    return false;
}

void AccountView::refreshAccounts()
{
    std::vector<Account> accounts = AccountManager::instance()->loadAccounts();

    mAccountList->setVisible(true);
    mNoAccountHint->setVisible(false);

    // Add new account if not
    std::vector<Account>::iterator iter;
    for (iter = accounts.begin(); iter != accounts.end(); iter++) {
        Account& account = *iter;
        if (!hasAccount(account)) {
            AccountItem *item = new AccountItem(account);
            accounts_list_.push_back(item);
        }
    }

    QVBoxLayout *layout = static_cast<QVBoxLayout*>(mAccountList->layout());

    for (std::vector<AccountItem*>::iterator item_iter = accounts_list_.begin();
         item_iter != accounts_list_.end(); item_iter++) {

        AccountItem *item = *item_iter;
        if (item->parent() == 0) {
            layout->addWidget(*item_iter);
        }
    }

    if (accounts.size() == 0) {
        mAccountList->setVisible(false);
        mNoAccountHint->setVisible(true);
    }
}
