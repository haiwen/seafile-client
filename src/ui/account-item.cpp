#include <QtGui>

#include "account-item.h"
#include "account-mgr.h"
#include "get-account-repos-dialog.h"

AccountItem::AccountItem(QWidget *parent, const Account& account)
    : QWidget(parent),
      account_(account)
{
    setupUi(this);

    mAccountPicture->setPixmap(QPixmap(":/images/account.png"));
    setAccount(account);

    connect(mListRepoBtn, SIGNAL(clicked()), this, SLOT(getAccountRepos()));
}

void AccountItem::setAccount(const Account& account)
{
    mUsername->setText(account.username);
    mServerAddr->setText(account.serverUrl.authority());
}

void AccountItem::getAccountRepos()
{
    GetAccountReposDialog dialog(account_, this);
    dialog.exec();
}
