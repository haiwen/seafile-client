#include <QMenu>
#include <QAction>
#include <QToolButton>
#include <QScopedPointer>

#include "account.h"
#include "account.h"
#include "seafile-applet.h"
#include "account-mgr.h"
#include "login-dialog.h"
#include "account-settings-dialog.h"
#include "rpc/rpc-client.h"
#include "main-window.h"
#include "init-vdrive-dialog.h"
#include "avatar-service.h"
#include "utils/paint-utils.h"

#include "account-view.h"

AccountView::AccountView(QWidget *parent)
    : QWidget(parent)
{
    setupUi(this);

    // Init account drop down menu
    account_menu_ = new QMenu;
    mAccountBtn->setMenu(account_menu_);
    mAccountBtn->setPopupMode(QToolButton::InstantPopup);

    onAccountChanged();

    connect(AvatarService::instance(), SIGNAL(avatarUpdated(const QString&, const QImage&)),
            this, SLOT(updateAvatar()));

    mAccountBtn->setCursor(Qt::PointingHandCursor);
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

void AccountView::editAccountSettings()
{
    const Account& account = seafApplet->accountManager()->currentAccount();

    AccountSettingsDialog dialog(account, this);

    dialog.exec();
}

void AccountView::updateAccountInfoDisplay()
{
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

    updateAvatar();
}

/**
 * Update the account menu when accounts changed
 */
void AccountView::onAccountChanged()
{
    const std::vector<Account>& accounts = seafApplet->accountManager()->accounts();

    // Remove all menu items
    account_menu_->clear();

    if (!accounts.empty()) {
        for (int i = 0, n = accounts.size(); i < n; i++) {
            Account account = accounts[i];
            QAction *action = makeAccountAction(accounts[i]);
            if (i == 0) {
                QIcon checked_icon = QIcon();
                checked_icon.addFile(":/images/account-checked.png", QSize(16, 16));
                checked_icon.addFile(":/images/account-checked@2x.png", QSize(32, 32));
                action->setIcon(checked_icon);
                action->setIconVisibleInMenu(true);
                // action->setChecked(true);
            }
            account_menu_->addAction(action);
        }

        account_menu_->addSeparator();
    }

    if (!accounts.empty()) {
        QIcon account_settings_icon = QIcon();
        account_settings_icon.addFile(":/images/account-settings.png", QSize(16, 16));
        account_settings_icon.addFile(":/images/account-settings@2x.png", QSize(32, 32));

        account_settings_action_ = new QAction(tr("Account settings"), this);
        account_settings_action_->setIcon(account_settings_icon);
        account_settings_action_->setIconVisibleInMenu(true);
        connect(account_settings_action_, SIGNAL(triggered()), this, SLOT(editAccountSettings()));
        account_menu_->addAction(account_settings_action_);
    }


    QIcon add_account_icon = QIcon();
    add_account_icon.addFile(":/images/add-account.png", QSize(16, 16));
    add_account_icon.addFile(":/images/add-account@2x.png", QSize(32, 32));

    // Add rest items
    add_account_action_ = new QAction(tr("Add an account"), this);
    add_account_action_->setIcon(add_account_icon);
    add_account_action_->setIconVisibleInMenu(true);
    connect(add_account_action_, SIGNAL(triggered()), this, SLOT(showAddAccountDialog()));
    account_menu_->addAction(add_account_action_);

    if (!accounts.empty()) {
        QIcon delet_account_icon = QIcon();
        delet_account_icon.addFile(":/images/delete-account.png", QSize(16, 16));
        delet_account_icon.addFile(":/images/delete-account@2x.png", QSize(32, 32));

        delete_account_action_ = new QAction(tr("Delete this account"), this);
        delete_account_action_->setIcon(delet_account_icon);
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
    // action->setCheckable(true);
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

void AccountView::updateAvatar()
{
    int w = ::getDPIScaledSize(32);
    mAccountBtn->setIconSize(QSize(w, w));
    const Account account = seafApplet->accountManager()->currentAccount();
    if (!account.isValid())  {
        mAccountBtn->setIcon(QIcon(":/images/account.png"));
        return;
    }

    AvatarService *service = AvatarService::instance();

    if (service->avatarFileExists(account.username)) {
        QString icon_path = AvatarService::instance()->getAvatarFilePath(account.username);
        mAccountBtn->setIcon(QIcon(icon_path));
        return;
    }

    mAccountBtn->setIcon(QIcon(":/images/account.png"));
    // will trigger a GetAvatarRequest
    service->getAvatar(account.username);
}
