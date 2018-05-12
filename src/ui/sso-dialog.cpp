#include <QDesktopServices>
#include <QRegExp>
#include <QtGui>

#include "account-mgr.h"
#include "settings-mgr.h"
#include "api/api-error.h"
#include "seafile-applet.h"
#include "ui/tray-icon.h"
#include "utils/api-utils.h"
#include "utils/utils.h"

#include "sso-dialog.h"

namespace
{
const char *kGetSSOStatusUrl = "api2/client-sso-link/%1/";
const char *kPreconfigureSSOLoginUrl = "PreconfigureSSOLoginUrl";
const int kSSOStatusCheckInterval = 3000;

QString getUUIDFromLink(const QString &link)
{
    QRegExp rx("/client-sso/([^/]+)/?$");
    if (rx.indexIn(link) < 0) {
        printf("invalid link %s\n", toCStr(link));
        return "";
    }
    return rx.cap(1);
}

} // anonymouse namespace

SSODialog::SSODialog(QWidget *parent)
    : QDialog(parent)
{
    setupUi(this);
    mLogo->setPixmap(QPixmap(":/images/seafile-32.png"));
    setWindowTitle(tr("%1 Single Sign On").arg(getBrand()));
    setWindowIcon(QIcon(":/images/seafile.png"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setStatusText(tr("Starting Single Sign On ..."));
    setStatusIcon(":/images/account.png");

    computer_name_ = seafApplet->settingsManager()->getComputerName();

    connect(mStatusText,
            SIGNAL(linkActivated(const QString &)),
            this,
            SLOT(openSSOLoginLinkInBrowser()));

    sso_check_timer_ = new QTimer(this);
    connect(sso_check_timer_, SIGNAL(timeout()), this, SLOT(ssoCheckTimerCB()));
    QTimer::singleShot(0, this, SLOT(start()));
}

void SSODialog::start()
{
    QString server_addr =
        seafApplet->readPreconfigureEntry(kPreconfigureSSOLoginUrl)
            .toString()
            .trimmed();
    if (!server_addr.isEmpty()) {
        if (QUrl(server_addr).isValid()) {
            qWarning("Using preconfigured SSO login url: %s\n",
                     toCStr(server_addr));
        } else {
            qWarning("Invalid preconfigured SSO login url: %s\n",
                     toCStr(server_addr));
            server_addr = "";
        }
    }

    QUrl url = server_addr;
    if (url.isEmpty()) {
        // When we reach here, there is no preconfigured SSO login url,
        // or the preconfigured url is invalid. So we ask the user for the url.
        server_addr = seafApplet->settingsManager()->getLastSSOUrl();
        if (!getSSOLoginUrl(server_addr, &url)) {
            reject();
            return;
        }
    }

    seafApplet->settingsManager()->setLastSSOUrl(url.toString());
    server_addr_ = url;
    getSSOLink();
}

bool SSODialog::getSSOLoginUrl(const QString& last_url, QUrl *url_out)
{
    QString server_addr = last_url;
    QUrl url;

    while (true) {
        bool ok;
        server_addr =
            seafApplet->getText(this,
                                tr("Single Sign On"),
                                tr("%1 Server Address").arg(getBrand()),
                                QLineEdit::Normal,
                                server_addr,
                                &ok);
        server_addr = server_addr.trimmed();

        // exit when user hits cancel button
        if (!ok) {
            return false;
        }

        if (server_addr.isEmpty()) {
            showWarning(tr("Server address must not be empty").arg(server_addr));
            continue;
        }

        if (!server_addr.startsWith("https://")) {
            showWarning(tr("%1 is not a valid server address. It has to start with 'https://'").arg(server_addr));
            continue;
        }

        url = QUrl(server_addr, QUrl::StrictMode);
        if (!url.isValid()) {
            showWarning(tr("%1 is not a valid server address").arg(server_addr));
            continue;
        }

        *url_out = url;
        return true;
    }
}

void SSODialog::setStatusText(const QString &status)
{
    mStatusText->setText(status);
}

void SSODialog::setStatusIcon(const QString &path)
{
    mStatusIcon->setPixmap(QPixmap(path));
}

void SSODialog::ensureVisible()
{
    show();
    raise();
    activateWindow();
}

void SSODialog::getSSOLink()
{
    get_link_request_.reset(new GetSSOLinkRequest(server_addr_.toString()));
    connect(get_link_request_.data(),
            SIGNAL(success(const QString &)),
            this,
            SLOT(onGetSSOLinkSuccess(const QString &)));
    connect(get_link_request_.data(),
            SIGNAL(failed(const ApiError &)),
            this,
            SLOT(onGetSSOLinkFailed(const ApiError &)));
    get_link_request_->send();
}

void SSODialog::onGetSSOLinkSuccess(const QString &link)
{
    qWarning() << "got SSO link" << link;
    sso_login_link_ = ::includeQueryParams(
                       link, ::getSeafileLoginParams(computer_name_, "shib_"));

    QString uuid = getUUIDFromLink(link);
    sso_status_link_ =
        ::urlJoin(server_addr_, QString(kGetSSOStatusUrl).arg(uuid));
    qWarning() << "SSO status link is" << sso_status_link_;
    setStatusText(
        tr("Please login in your browser with %1")
            .arg(QString("<a style=\"color:#F89A01\" href=\"#\">%1</a>")
                     .arg(tr("this link"))));
    openSSOLoginLinkInBrowser();
    startSSOStatusCheck();
}

void SSODialog::onGetSSOLinkFailed(const ApiError &error)
{
    qWarning() << ("get SSO link failed: ") << error.toString();
    fail(tr("Failed to get SSO link:\n%1").arg(error.toString()));
}

void SSODialog::fail(const QString &msg)
{
    ensureVisible();
    seafApplet->warningBox(msg, this);
    reject();
}

void SSODialog::startSSOStatusCheck()
{
    sendSSOStatusRequest();
    sso_check_timer_->start(kSSOStatusCheckInterval);
}


void SSODialog::sendSSOStatusRequest()
{
    status_check_request_.reset(new GetSSOStatusRequest(sso_status_link_));
    connect(
        status_check_request_.data(),
        SIGNAL(success(const QString &, const QString &, const QString &)),
        this,
        SLOT(onSSOSuccess(const QString &, const QString &, const QString &)));
    connect(status_check_request_.data(),
            SIGNAL(failed(const ApiError &)),
            this,
            SLOT(onSSOFailed(const ApiError &)));
    status_check_request_->send();
}

void SSODialog::ssoCheckTimerCB()
{
    if (status_check_request_) {
        qDebug("SSO status check request in progress");
        return;
    } else {
        sendSSOStatusRequest();
    }
}

void SSODialog::onSSOSuccess(const QString &status,
                             const QString &email,
                             const QString &apikey)
{
    qDebug() << "SSO status =" << status << ", email =" << email
             << ", api key =" << apikey;
    if (status == "waiting") {
        qDebug("SSO status: waiting");
        status_check_request_.reset();
    } else if (status == "success") {
        sso_check_timer_->stop();
        getAccountInfo(Account(server_addr_, email, apikey, 0, true));
    }
}

void SSODialog::onSSOFailed(const ApiError &error)
{
    sso_check_timer_->stop();
    fail(tr("Failed to get SSO status:\n%1").arg(error.toString()));
}

void SSODialog::getAccountInfo(const Account &account)
{
    setStatusText(tr("Fetching account information ..."));
    account_info_req_.reset(new FetchAccountInfoRequest(account));
    connect(account_info_req_.data(),
            SIGNAL(success(const AccountInfo &)),
            this,
            SLOT(onFetchAccountInfoSuccess(const AccountInfo &)));
    connect(account_info_req_.data(),
            SIGNAL(failed(const ApiError &)),
            this,
            SLOT(onFetchAccountInfoFailed(const ApiError &)));
    account_info_req_->send();
}

void SSODialog::onFetchAccountInfoSuccess(const AccountInfo &info)
{
    Account account = account_info_req_->account();
    // The user may use the username to login, but we need to store the email
    // to account database
    account.username = info.email;
    if (seafApplet->accountManager()->saveAccount(account) < 0) {
        fail(tr("Failed to save current account"));
        return;
    }

    seafApplet->accountManager()->updateAccountInfo(account, info);
    ensureVisible();
    QString buf = tr("Successfully logged in as %1").arg(info.name);
    seafApplet->trayIcon()->showMessage(getBrand(), buf);
    accept();
}

void SSODialog::onFetchAccountInfoFailed(const ApiError &error)
{
    fail(tr("Failed to get account information:\n%1").arg(error.toString()));
}

void SSODialog::openSSOLoginLinkInBrowser()
{
    QDesktopServices::openUrl(sso_login_link_);
}

void SSODialog::closeEvent(QCloseEvent *event)
{
    if (seafApplet->yesOrNoBox(
            tr("Login not finished yet. Really quit?"), this, false)) {
        reject();
    } else {
        event->ignore();
    }
}

void SSODialog::showWarning(const QString& msg)
{
    seafApplet->warningBox(msg, this);
}
