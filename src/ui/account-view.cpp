#include <QMenu>
#include <QAction>
#include <QToolButton>
#include <QScopedPointer>
#include <QPainter>
#include <QStringList>
#include <QDesktopServices>
#include <QMouseEvent>
#include <QUrl>
#include <QUrlQuery>
#include <QThreadPool>

#include "account.h"
#include "seafile-applet.h"
#include "account-mgr.h"
#include "login-dialog.h"
#include "settings-mgr.h"
#ifdef HAVE_SHIBBOLETH_SUPPORT
#include "shib/shib-login-dialog.h"
#endif // HAVE_SHIBBOLETH_SUPPORT
#include "account-settings-dialog.h"
#include "rpc/rpc-client.h"
#include "main-window.h"
#include "init-vdrive-dialog.h"
#include "auto-login-service.h"
#include "avatar-service.h"
#include "utils/paint-utils.h"
#include "filebrowser/file-browser-manager.h"
#include "api/api-error.h"
#include "api/requests.h"
#include "filebrowser/auto-update-mgr.h"
#include "repo-service.h"

#include "account-view.h"
namespace {

} // namespace

AccountView::AccountView(QWidget *parent)
    : QWidget(parent)
{
    setupUi(this);

    // Init account drop down menu
    account_menu_ = new QMenu;
    mAccountBtn->setMenu(account_menu_);
    mAccountBtn->setPopupMode(QToolButton::InstantPopup);
    mAccountBtn->setFixedSize(QSize(AvatarService::kAvatarSize, AvatarService::kAvatarSize));

    onAccountChanged();

    connect(AvatarService::instance(), SIGNAL(avatarUpdated(const QString&, const QImage&)),
            this, SLOT(updateAvatar()));

    mAccountBtn->setCursor(Qt::PointingHandCursor);
    mAccountBtn->installEventFilter(this);
    account_menu_->installEventFilter(this);

    connect(seafApplet->accountManager(), SIGNAL(requireAddAccount()),
            this, SLOT(showAddAccountDialog()));
    connect(mServerAddr, SIGNAL(linkActivated(const QString&)),
            this, SLOT(visitServerInBrowser(const QString&)));

    // Must get the pixmap from QIcon because QIcon would load the 2x version
    // automatically.
    mRefreshLabel->setPixmap(QIcon(":/images/toolbar/refresh-new.png").pixmap(20));
    mRefreshLabel->installEventFilter(this);
}

void AccountView::showAddAccountDialog()
{
    LoginDialog dialog(this);
    // Show InitVirtualDriveDialog for the first account added
    AccountManager *account_mgr = seafApplet->accountManager();
    if (dialog.exec() == QDialog::Accepted
        && account_mgr->accounts().size() == 1) {

        InitVirtualDriveDialog dialog(account_mgr->currentAccount(), seafApplet->mainWindow());
#if defined(Q_OS_WIN32)
        dialog.exec();
#endif
    }
}

void AccountView::deleteAccount()
{
    QAction *action = qobject_cast<QAction*>(sender());
    if (!action)
        return;
    Account account = qvariant_cast<Account>(action->data());

    // QString question = tr("Are you sure to remove account from \"%1\"?<br>"
    //                       "<b>Warning: All libraries of this account would be unsynced!</b>").arg(account.serverUrl.toString());

    QString question = tr("Are you sure you want to remove account %1?<br><br>"
                          "The account will be removed locally. All syncing "
                          "configuration will be removed too. The account at "
                          "the server will not be affected.")
                           .arg(account.username);

    if (seafApplet->yesOrNoBox(question, this, false)) {
        FileBrowserManager::getInstance()->closeAllDialogByAccount(account);
        QString error, server_addr = account.serverUrl.host();
        if (seafApplet->rpcClient()->unsyncReposByAccount(server_addr,
                                                          account.username,
                                                          &error) < 0) {

            seafApplet->warningBox(
                tr("Failed to unsync libraries of this account: %1").arg(error),
                this);
        }

        seafApplet->accountManager()->removeAccount(account);
    }
}

void AccountView::editAccountSettings()
{
    QAction *action = qobject_cast<QAction*>(sender());
    if (!action)
        return;
    Account account = qvariant_cast<Account>(action->data());

    AccountSettingsDialog dialog(account, this);

    dialog.exec();
}

void AccountView::updateAccountInfoDisplay()
{
    if (seafApplet->accountManager()->hasAccount()) {
        const Account account = seafApplet->accountManager()->currentAccount();
        if (!account.accountInfo.name.isEmpty()) {
            mEmail->setText(account.accountInfo.name);
        } else {
            mEmail->setText(account.username);
        }
        // mServerAddr->setOpenExternalLinks(true);
        mServerAddr->setToolTip(tr("click to open the website"));

        QString host = account.serverUrl.host();
        QString href = account.serverUrl.toString();
        QString text = QString("<a style="
                               "\"color:#A4A4A4; text-decoration: none;\" "
                               "href=\"%1\">%2</a>").arg(href).arg(host);

        mServerAddr->setText(account.isPro() ? QString("%1 <small>%2<small>").arg(text).arg(tr("pro version")) : text);
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
        for (size_t i = 0, n = accounts.size(); i < n; i++) {
            const Account &account = accounts[i];
            QString text = account.username + "(" + account.serverUrl.host() + ")";
            if (!account.isValid()) {
                text += ", " + tr("not logged in");
            }
            QMenu *submenu = new QMenu(text, account_menu_);
            if (i == 0) {
                submenu->setIcon(QIcon(":/images/account-checked.png"));
            } else {
                submenu->setIcon(QIcon(":/images/account-else.png"));
            }

            QAction *submenu_action = submenu->menuAction();
            submenu_action->setData(QVariant::fromValue(account));
            connect(submenu_action, SIGNAL(triggered()), this, SLOT(onAccountItemClicked()));

            QAction *action = new QAction(tr("Choose"), submenu);
            action->setIcon(QIcon(":/images/account-checked.png"));
            action->setIconVisibleInMenu(true);
            action->setData(QVariant::fromValue(account));
            connect(action, SIGNAL(triggered()), this, SLOT(onAccountItemClicked()));

            submenu->addAction(action);
            submenu->setDefaultAction(action);

            QAction *account_settings_action = new QAction(tr("Account settings"), this);
            account_settings_action->setIcon(QIcon(":/images/account-settings.png"));
            account_settings_action->setIconVisibleInMenu(true);
            account_settings_action->setData(QVariant::fromValue(account));
            connect(account_settings_action, SIGNAL(triggered()), this, SLOT(editAccountSettings()));
            submenu->addAction(account_settings_action);

            QAction *toggle_action = new QAction(this);
            toggle_action->setIcon(QIcon(":/images/logout.png"));
            toggle_action->setIconVisibleInMenu(true);
            toggle_action->setData(QVariant::fromValue(account));
            connect(toggle_action, SIGNAL(triggered()), this, SLOT(toggleAccount()));
            if (account.isValid())
                toggle_action->setText(tr("Logout"));
            else
                toggle_action->setText(tr("Login"));
            submenu->addAction(toggle_action);

            QAction *delete_account_action = new QAction(tr("Delete"), this);
            delete_account_action->setIcon(QIcon(":/images/delete-account.png"));
            delete_account_action->setIconVisibleInMenu(true);
            delete_account_action->setData(QVariant::fromValue(account));
            connect(delete_account_action, SIGNAL(triggered()), this, SLOT(deleteAccount()));
            submenu->addAction(delete_account_action);

            account_menu_->addMenu(submenu);
        }

        account_menu_->addSeparator();
    }

    add_account_action_ = new QAction(tr("Add an account"), this);
    add_account_action_->setIcon(QIcon(":/images/add-account.png"));
    add_account_action_->setIconVisibleInMenu(true);
    connect(add_account_action_, SIGNAL(triggered()), this, SLOT(showAddAccountDialog()));
    account_menu_->addAction(add_account_action_);

    updateAccountInfoDisplay();
}

QAction* AccountView::makeAccountAction(const Account& account)
{
    QString text = account.username + "(" + account.serverUrl.host() + ")";
    if (!account.isValid()) {
        text += ", " + tr("not logged in");
    }
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

    if (!account.isValid()) {
        seafApplet->accountManager()->reloginAccount(account);
    } else {
        seafApplet->accountManager()->setCurrentAccount(account);
    }
}

void AccountView::updateAvatar()
{
    mAccountBtn->setIconSize(QSize(AvatarService::kAvatarSize, AvatarService::kAvatarSize));
    const Account account = seafApplet->accountManager()->currentAccount();
    if (!account.isValid())  {
        mAccountBtn->setIcon(QIcon(":/images/account.png"));
        return;
    }

    AvatarService *service = AvatarService::instance();
    QIcon avatar = QPixmap::fromImage(service->getAvatar(account.username));
    mAccountBtn->setIcon(QIcon(avatar));
}

bool AccountView::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == account_menu_ && event->type() == QEvent::MouseButtonRelease) {
        QMouseEvent *ev = (QMouseEvent*)event;
        QAction *action = account_menu_->actionAt(ev->pos());
        if (action) {
            action->trigger();
        }
    }
    if (obj == mAccountBtn && event->type() == QEvent::Paint) {
        QRect rect(0, 0, AvatarService::kAvatarSize, AvatarService::kAvatarSize);
        QPainter painter(mAccountBtn);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::HighQualityAntialiasing);

        // get the device pixel radio from current painter device
        double scale_factor = globalDevicePixelRatio();

        QPixmap image(mAccountBtn->icon().pixmap(rect.size()).scaled(scale_factor * rect.size()));
        QRect actualRect(QPoint(0, 0),
                         QSize(AvatarService::kAvatarSize * scale_factor,
                               AvatarService::kAvatarSize * scale_factor));

        QImage masked_image(actualRect.size(),
                            QImage::Format_ARGB32_Premultiplied);
        masked_image.fill(Qt::transparent);
        QPainter mask_painter;
        mask_painter.begin(&masked_image);
        mask_painter.setRenderHint(QPainter::Antialiasing);
        mask_painter.setRenderHint(QPainter::HighQualityAntialiasing);
        mask_painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
        mask_painter.setPen(Qt::NoPen);
        mask_painter.setBrush(Qt::white);
        mask_painter.drawEllipse(actualRect);
        mask_painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
        mask_painter.drawPixmap(actualRect, image);
        mask_painter.setCompositionMode(QPainter::CompositionMode_DestinationOver);
        mask_painter.fillRect(actualRect, Qt::transparent);
        mask_painter.end();
        masked_image.setDevicePixelRatio(scale_factor);

        painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
        painter.drawImage(QPoint(0,0), masked_image);
        return true;
    }
    if (obj == mRefreshLabel) {
        if (event->type() == QEvent::MouseButtonPress) {
            emit refresh();
            return true;
        } else if (event->type() == QEvent::Enter) {
            mRefreshLabel->setPixmap(QIcon(":/images/toolbar/refresh-orange.png").pixmap(20));
            return true;
        } else if (event->type() == QEvent::Leave) {
            mRefreshLabel->setPixmap(QIcon(":/images/toolbar/refresh-new.png").pixmap(20));
            return true;
        }
    }
    return QObject::eventFilter(obj, event);
}

/**
 * Only remove the api token of the account. The accout would still be shown
 * in the account list.
 */
void AccountView::toggleAccount()
{
    QAction *action = qobject_cast<QAction*>(sender());
    if (!action)
        return;
    Account account = qvariant_cast<Account>(action->data());
    if (!account.isValid()) {
        seafApplet->accountManager()->reloginAccount(account);
        return;
    }

    qWarning("Logging out current account %s", account.username.toUtf8().data());
    AutoUpdateManager::instance()->cleanCachedFile();

    // logout Account
    FileBrowserManager::getInstance()->closeAllDialogByAccount(account);
    seafApplet->accountManager()->logoutDevice(account);
}

void AccountView::visitServerInBrowser(const QString& link)
{
    AutoLoginService::instance()->startAutoLogin("/");
}
