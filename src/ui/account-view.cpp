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

    refreshAccounts();
}

QVBoxLayout* AccountView::getLayout()
{
    return static_cast<QVBoxLayout*>(layout());
}

void AccountView::showAddAccountDialog()
{
    LoginDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        refreshAccounts();
    }
}

void AccountView::refreshAccounts()
{
    std::vector<Account> accounts = AccountManager::instance()->loadAccounts();
    if (accounts.size() == 0) {
        mNoAccountHint->show();
        return;
    }

    mNoAccountHint->hide();
    std::vector<Account>::iterator iter;
    for (iter = accounts.begin(); iter != accounts.end(); iter++) {
        AccountItem *item = new AccountItem(this, *iter);
        getLayout()->insertWidget(0, item);
    }
}
