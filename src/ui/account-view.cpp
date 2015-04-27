#include <QMenu>
#include <QAction>
#include <QToolButton>
#include <QScopedPointer>
#include <QPainter>
#include <QStringList>
#include <QDesktopServices>
#include <QUrl>
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
#include <QUrlQuery>
#endif

#include "account.h"
#include "seafile-applet.h"
#include "account-mgr.h"
#include "login-dialog.h"
#include "account-settings-dialog.h"
#include "rpc/rpc-client.h"
#include "rpc/local-repo.h"
#include "main-window.h"
#include "init-vdrive-dialog.h"
#include "avatar-service.h"
#include "utils/paint-utils.h"
#include "filebrowser/file-browser-manager.h"
#include "api/api-error.h"
#include "api/requests.h"

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

    connect(seafApplet->accountManager(), SIGNAL(accountRequireRelogin(const Account&)),
            this, SLOT(reloginAccount(const Account &)));
    connect(mServerAddr, SIGNAL(linkActivated(const QString&)),
            this, SLOT(visitServerInBrowser(const QString&)));
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
    QString question = tr("Are you sure to remove this account?<br>"
                          "<b>Warning: All libraries of this account would be unsynced!</b>");

    if (seafApplet->yesOrNoBox(question, this, false)) {
        const Account& account = seafApplet->accountManager()->currentAccount();
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
    const Account& account = seafApplet->accountManager()->currentAccount();

    AccountSettingsDialog dialog(account, this);

    dialog.exec();
}

void AccountView::updateAccountInfoDisplay()
{
    if (seafApplet->accountManager()->hasAccount()) {
        const Account account = seafApplet->accountManager()->currentAccount();
        mEmail->setText(account.username);
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
            Account account = accounts[i];
            QAction *action = makeAccountAction(accounts[i]);
            if (i == 0) {
                action->setIcon(QIcon(":/images/account-checked.png"));
                action->setIconVisibleInMenu(true);
            }
            account_menu_->addAction(action);
        }

        account_menu_->addSeparator();
    }

    if (!accounts.empty()) {
        account_settings_action_ = new QAction(tr("Account settings"), this);
        account_settings_action_->setIcon(QIcon(":/images/account-settings.png"));
        account_settings_action_->setIconVisibleInMenu(true);
        connect(account_settings_action_, SIGNAL(triggered()), this, SLOT(editAccountSettings()));
        account_menu_->addAction(account_settings_action_);
    }

    add_account_action_ = new QAction(tr("Add an account"), this);
    add_account_action_->setIcon(QIcon(":/images/add-account.png"));
    add_account_action_->setIconVisibleInMenu(true);
    connect(add_account_action_, SIGNAL(triggered()), this, SLOT(showAddAccountDialog()));
    account_menu_->addAction(add_account_action_);

    if (!accounts.empty()) {
        logout_action_ = new QAction(tr("Logout"), this);
        logout_action_->setIcon(QIcon(":/images/logout.png"));
        logout_action_->setIconVisibleInMenu(true);
        connect(logout_action_, SIGNAL(triggered()), this, SLOT(logoutAccount()));
        account_menu_->addAction(logout_action_);

        delete_account_action_ = new QAction(tr("Delete this account"), this);
        delete_account_action_->setIcon(QIcon(":/images/delete-account.png"));
        delete_account_action_->setIconVisibleInMenu(true);
        connect(delete_account_action_, SIGNAL(triggered()), this, SLOT(deleteAccount()));
        account_menu_->addAction(delete_account_action_);
    }

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
    GetRepoTokensRequest *req = new GetRepoTokensRequest(
        account, repo_ids);

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

    if (service->avatarFileExists(account.username)) {
        QString icon_path = AvatarService::instance()->getAvatarFilePath(account.username);
        mAccountBtn->setIcon(QIcon(icon_path));
        return;
    }

    mAccountBtn->setIcon(QIcon(":/images/account.png"));
    // will trigger a GetAvatarRequest
    service->getAvatar(account.username);
}

bool AccountView::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == mAccountBtn && event->type() == QEvent::Paint) {
        QRect rect(0, 0, AvatarService::kAvatarSize, AvatarService::kAvatarSize);
        QPainter painter(mAccountBtn);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::HighQualityAntialiasing);

        // get the device pixel radio from current painter device
        int scale_factor = 1;
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
        scale_factor = painter.device()->devicePixelRatio();
#endif // QT5

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
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
        masked_image.setDevicePixelRatio(scale_factor);
#endif // QT5

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
void AccountView::logoutAccount()
{
    QString question = tr("Are you sure to logout this account?");

    if (!seafApplet->yesOrNoBox(question, this, false)) {
        return;
    }
    const Account& account = seafApplet->accountManager()->currentAccount();
    FileBrowserManager::getInstance()->closeAllDialogByAccount(account);
    LogoutDeviceRequest *req = new LogoutDeviceRequest(account);
    connect(req, SIGNAL(success()),
            this, SLOT(onLogoutDeviceRequestSuccess()));
    connect(req, SIGNAL(failed(const ApiError&)),
            this, SLOT(onLogoutDeviceRequestFailed(const ApiError&)));
    req->send();
}

void AccountView::reloginAccount(const Account &account)
{
    LoginDialog dialog(this);
    dialog.initFromAccount(account);
    if (dialog.exec() == QDialog::Accepted) {
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

void AccountView::onLogoutDeviceRequestFailed(const ApiError& error)
{
    LogoutDeviceRequest *req = (LogoutDeviceRequest *)QObject::sender();
    req->deleteLater();
    QString msg;
    if (error.httpErrorCode() == 404) {
        msg = tr("Logging out is not supported on your server (version too low).");
    } else {
        msg = tr("Failed to remove information on server: %1").arg(error.toString());
    }
    seafApplet->warningBox(msg, this);
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
    LogoutDeviceRequest *req = (LogoutDeviceRequest *)QObject::sender();
    req->deleteLater();
    seafApplet->warningBox(
        tr("Failed to get repo sync information from server: %1").arg(error.toString()), this);
}

void AccountView::visitServerInBrowser(const QString& link)
{
    const Account& account = seafApplet->accountManager()->currentAccount();

    // TODO: uncomment the following check
    // if (!account.isAtLeastVersion(4, 2, 0)) {
    //     QDesktopServices::openUrl(account.serverUrl);
    //     return;
    // }

    GetLoginTokenRequest *req = new GetLoginTokenRequest(account);

    connect(req, SIGNAL(success(const QString&)),
            this, SLOT(onGetLoginTokenSuccess(const QString&)));

    connect(req, SIGNAL(failed(const ApiError&)),
            this, SLOT(onGetLoginTokenFailed(const ApiError&)));

    req->send();
}

void AccountView::onGetLoginTokenSuccess(const QString& token)
{
    GetLoginTokenRequest *req = (GetLoginTokenRequest *)(sender());
    printf("login token is %s\n", token.toUtf8().data());

    QUrl url = req->account().getAbsoluteUrl("/client-login/");
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    QUrlQuery q;
    q.addQueryItem("token", token);
    url.setQuery(q);
#else
    url.addQueryItem("token", token);
#endif
    printf("url is %s\n", url.toEncoded().data());
    QDesktopServices::openUrl(url);
    req->deleteLater();
}

void AccountView::onGetLoginTokenFailed(const ApiError& error)
{
    GetLoginTokenRequest *req = (GetLoginTokenRequest *)(sender());
    printf("get login token failed: %s\n", error.toString().toUtf8().data());
    // server doesn't support client directly login, or other errors happened.
    // We open the server url directly in this case;
    QDesktopServices::openUrl(req->account().serverUrl);
    req->deleteLater();
}
