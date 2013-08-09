#include <QtGui>
#include "account-item.h"
#include "account-mgr.h"

AccountItem::AccountItem(QWidget *parent, const Account& account) : QWidget(parent)
{
    setupUi(this);

    mAccountPicture->setPixmap(QPixmap(":/images/account.png"));
    setAccount(account);
}

void AccountItem::setAccount(const Account& account)
{
    if (account_ == 0) {
        account_ = new Account(account);
    }

    mUsername->setText(account.username);
    mServerAddr->setText(account.serverUrl.authority());
}
