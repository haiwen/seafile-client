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
#include "rpc/local-repo.h"
#include "main-window.h"
#include "init-vdrive-dialog.h"
#include "auto-login-service.h"
#include "avatar-service.h"
#include "utils/paint-utils.h"
#include "filebrowser/file-browser-manager.h"
#include "api/api-error.h"
#include "api/requests.h"
#include "filebrowser/auto-update-mgr.h"

#include "account-view.h"
namespace {

QStringList collectSyncedReposForAccount(const Account& account)
{
    std::vector<LocalRepo> repos;
    SeafileRpcClient *rpc = seafApplet->rpcClient();
    rpc->listLocalRepos(&repos);
    QStringList repo_ids;
    for (size_t i = 0; i < repos.size(); i++) {
        LocalRepo repo = repos[i];
        QString repo_server_url;
        if (rpc->getRepoProperty(repo.id, "server-url", &repo_server_url) < 0) {
            continue;
        }
        if (QUrl(repo_server_url).host() != account.serverUrl.host()) {
            continue;
        }
        QString token;
        if (rpc->getRepoProperty(repo.id, "token", &token) < 0 || token.isEmpty()) {
            repo_ids.append(repo.id);
        }
    }

    return repo_ids;
}

}

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

    connect(seafApplet->accountManager(), SIGNAL(accountRequireRelogin(const Account&)),
            this, SLOT(reloginAccount(const Account &)));
    connect(seafApplet->accountManager(), SIGNAL(requireAddAccount()),
            this, SLOT(showAddAccountDialog()));
    connect(mServerAddr, SIGNAL(linkActivated(const QString&)),
            this, SLOT(visitServerInBrowser(const QString&)));

    mRefreshBtn->setIcon(QIcon(":/images/toolbar/refresh-alpha.png"));
    mRefreshBtn->setIconSize(QSize(20, 20));
    connect(mRefreshBtn, SIGNAL(clicked()), this, SIGNAL(refresh()));
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
        reloginAccount(account);
    } else {
        seafApplet->accountManager()->setCurrentAccount(account);
    }
}

void AccountView::getRepoTokenWhenRelogin(const Account& account)
{
    QStringList repo_ids = collectSyncedReposForAccount(account);
    if (repo_ids.empty()) {
        return;
    }

    /* old account object don't contains the new token */
    QString host = account.serverUrl.host();
    QString username = account.username;
    Account new_account = seafApplet->accountManager()->getAccountByHostAndUsername(host, username);
    if (!new_account.isValid())
        return;

    // For debugging lots of repos problem.
    // TODO: Comment this out before committing!!
    //
    // int targetNumberForDebug = 300;
    // while (repo_ids.size() < targetNumberForDebug) {
    //     repo_ids.append(repo_ids);
    // }
    // repo_ids = repo_ids.mid(0, 300);
    // printf ("repo_ids.size() = %d\n", repo_ids.size());

    GetRepoTokensRequest *req = new GetRepoTokensRequest(
        new_account, repo_ids);

    connect(req, SIGNAL(success()),
            this, SLOT(onGetRepoTokensSuccess()));
    connect(req, SIGNAL(failed(const ApiError&)),
            this, SLOT(onGetRepoTokensFailed(const ApiError&)));
    req->send();
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
        QRect actualRect(QPoint(0, 0), image.size());

        QImage masked_image(actualRect.size(), QImage::Format_ARGB32_Premultiplied);
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
        reloginAccount(account);
        return;
    }

    AutoUpdateManager::instance()->cleanCachedFile();

    // logout Account
    FileBrowserManager::getInstance()->closeAllDialogByAccount(account);
    LogoutDeviceRequest *req = new LogoutDeviceRequest(account);
    connect(req, SIGNAL(success()),
            this, SLOT(onLogoutDeviceRequestSuccess()));
    connect(req, SIGNAL(failed(const ApiError&)),
            this, SLOT(onLogoutDeviceRequestSuccess()));
    req->send();
}

void AccountView::reloginAccount(const Account &account)
{
    bool accepted;
    do {
#ifdef HAVE_SHIBBOLETH_SUPPORT
        if (account.isShibboleth) {
            ShibLoginDialog shib_dialog(account.serverUrl, seafApplet->settingsManager()->getComputerName(), this);
            accepted = shib_dialog.exec() == QDialog::Accepted;
            break;
        }
#endif
        LoginDialog dialog(this);
        dialog.initFromAccount(account);
        accepted = dialog.exec() == QDialog::Accepted;
    } while (0);

    if (accepted) {
        getRepoTokenWhenRelogin(account);
    }
}

void AccountView::onLogoutDeviceRequestSuccess()
{
    LogoutDeviceRequest *req = (LogoutDeviceRequest *)QObject::sender();
    const Account& account = req->account();
    QString error;
    if (seafApplet->rpcClient()->removeSyncTokensByAccount(account.serverUrl.host(),
                                                           account.username,
                                                           &error)  < 0) {
        seafApplet->warningBox(
            tr("Failed to remove local repos sync token: %1").arg(error),
            this);
        return;
    }
    seafApplet->accountManager()->clearAccountToken(account);

    req->deleteLater();
}

void AccountView::onGetRepoTokensSuccess()
{
    GetRepoTokensRequest *req = (GetRepoTokensRequest *)(sender());
    foreach (const QString& repo_id, req->repoTokens().keys()) {
        seafApplet->rpcClient()->setRepoToken(
            repo_id, req->repoTokens().value(repo_id));
    }
    req->deleteLater();
}

void AccountView::onGetRepoTokensFailed(const ApiError& error)
{
    GetRepoTokensRequest *req = (GetRepoTokensRequest *)QObject::sender();
    req->deleteLater();
    seafApplet->warningBox(
        tr("Failed to get repo sync information from server: %1").arg(error.toString()), this);
}

void AccountView::visitServerInBrowser(const QString& link)
{
    AutoLoginService::instance()->startAutoLogin("/");
}
