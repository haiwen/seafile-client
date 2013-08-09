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

void AccountView::refreshAccounts()
{
    QVBoxLayout *layout = static_cast<QVBoxLayout*>(mAccountList->layout());
    QLayoutItem *child;
    while ((child = layout->takeAt(0)) != 0) {
        delete child;
    }

    std::vector<Account> accounts = AccountManager::instance()->loadAccounts();
    if (accounts.size() == 0) {
        mNoAccountHint->setVisible(true);
        return;
    }

    mNoAccountHint->setVisible(false);
    std::vector<Account>::iterator iter;
    for (iter = accounts.begin(); iter != accounts.end(); iter++) {
        AccountItem *item = new AccountItem(this, *iter);
        layout->insertWidget(0, item);
    }
}
