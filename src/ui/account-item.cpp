#include <QtGui>

#include "account-item.h"
#include "account-mgr.h"
#include "list-account-repos-dialog.h"

AccountItem::AccountItem(const Account& account, QWidget *parent)
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
    ListAccountReposDialog dialog(account_, this);
    dialog.exec();
}
