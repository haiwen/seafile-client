#include <QMenu>
#include <QAction>
#include <QToolButton>

#include "QtAwesome.h"
#include "account.h"
#include "account.h"
#include "seafile-applet.h"
#include "account-mgr.h"
#include "login-dialog.h"
#include "rpc/rpc-client.h"
#include "main-window.h"
#include "init-vdrive-dialog.h"

#include "account-view.h"

AccountView::AccountView(QWidget *parent)
    : QWidget(parent)
{
    setupUi(this);

    mSeahubMessagesBtn->setVisible(false);

    // Init account drop down menu
    account_menu_ = new QMenu;
    mAccountBtn->setMenu(account_menu_);
    mAccountBtn->setPopupMode(QToolButton::InstantPopup);

    onAccountsChanged();
}

void AccountView::showAddAccountDialog()
{
    LoginDialog dialog(this);
    // Show InitVirtualDriveDialog for the first account added
    AccountManager *account_mgr = seafApplet->accountManager();
    if (dialog.exec() == QDialog::Accepted
        && account_mgr->accounts().size() == 1) {

        InitVirtualDriveDialog dialog(account_mgr->currentAccount(), seafApplet->mainWindow());
#if defined(Q_WS_WIN)
        dialog.exec();
#endif
    }
}

void AccountView::deleteAccount()
{
    QString question = tr("Are you sure to remove this account?<br>"
                          "<b>Warning: All libraries of this account would be unsynced!</b>");

    if (seafApplet->yesOrNoBox(question, this, false)) {
        const Account& account = seafApplet->accountManager()->currentAccount();
        QString error, server_addr = account.serverUrl.host();
        if (seafApplet->rpcClient()->unsyncReposByAccount(server_addr,
                                                          account.username,
                                                          &error) < 0) {

            seafApplet->warningBox(tr("Failed to unsync libraries of this account: %1").arg(error));
        }

        seafApplet->accountManager()->removeAccount(account);
    }
}

void AccountView::updateAccountInfoDisplay()
{
    mAccountBtn->setIcon(QIcon(":/images/account.png"));
    mAccountBtn->setIconSize(QSize(32, 32));
    if (seafApplet->accountManager()->hasAccount()) {
        const Account account = seafApplet->accountManager()->currentAccount();
        mEmail->setText(account.username);
        mServerAddr->setOpenExternalLinks(true);
        mServerAddr->setToolTip(tr("click to open the website"));

        QString host = account.serverUrl.host();
        QString href = account.serverUrl.toString();
        QString text = QString("<a style="
                               "\"color:#A4A4A4; text-decoration: none;\" "
                               "href=\"%1\">%2</a>").arg(href).arg(host);

        mServerAddr->setText(text);
    } else {
        mEmail->setText(tr("No account"));
        mServerAddr->setText(QString());
    }
}

/**
 * Update the account menu when accounts changed
 */
void AccountView::onAccountsChanged()
{
    const std::vector<Account>& accounts = seafApplet->accountManager()->accounts();

    // Remove all menu items
    account_menu_->clear();

    if (!accounts.empty()) {
        for (int i = 0, n = accounts.size(); i < n; i++) {
            Account account = accounts[i];
            QAction *action = makeAccountAction(accounts[i]);
            if (i == 0) {
                action->setChecked(true);
            }
            account_menu_->addAction(action);
        }

        account_menu_->addSeparator();
    }

    // Add rest items
    add_account_action_ = new QAction(tr("Add an account"), this);
    add_account_action_->setIcon(awesome->icon(icon_plus));
    add_account_action_->setIconVisibleInMenu(true);
    connect(add_account_action_, SIGNAL(triggered()), this, SLOT(showAddAccountDialog()));
    account_menu_->addAction(add_account_action_);

    if (!accounts.empty()) {
        delete_account_action_ = new QAction(tr("Delete this account"), this);
        delete_account_action_->setIcon(awesome->icon(icon_remove));
        delete_account_action_->setIconVisibleInMenu(true);
        connect(delete_account_action_, SIGNAL(triggered()), this, SLOT(deleteAccount()));
        account_menu_->addAction(delete_account_action_);
    }

    updateAccountInfoDisplay();
}

QAction* AccountView::makeAccountAction(const Account& account)
{
    QString text = account.username + "(" + account.serverUrl.host() + ")";
    QAction *action = new QAction(text, account_menu_);
    action->setData(QVariant::fromValue(account));
    action->setCheckable(true);
    // QMenu won't display tooltip for menu item
    // action->setToolTip(account.serverUrl.host());

    connect(action, SIGNAL(triggered()), this, SLOT(onAccountItemClicked()));

    return action;
}

// Switch to the clicked account in the account menu
void AccountView::onAccountItemClicked()
{
    QAction *action = (QAction *)(sender());
    Account account = qvariant_cast<Account>(action->data());

    seafApplet->accountManager()->setCurrentAccount(account);
}
