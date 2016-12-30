#include <QtGlobal>

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
#include <QtWidgets>
#else
#include <QtGui>
#endif
#include <QtNetwork>
#include <QInputDialog>
#include <QStringList>
#include <QSettings>

#include "settings-mgr.h"
#include "account-mgr.h"
#include "seafile-applet.h"
#include "api/api-error.h"
#include "api/requests.h"
#include "login-dialog.h"
#include "utils/utils.h"
#ifdef HAVE_SHIBBOLETH_SUPPORT
#include "shib/shib-login-dialog.h"
#endif // HAVE_SHIBBOLETH_SUPPORT

namespace {

const char *kDefaultServerAddr1 = "https://seacloud.cc";
const char *kUsedServerAddresses = "UsedServerAddresses";
const char *const kPreconfigureServerAddr = "PreconfigureServerAddr";
const char *const kPreconfigureServerAddrOnly = "PreconfigureServerAddrOnly";

const char *const kSeafileOTPHeader = "X-Seafile-OTP";

QStringList getUsedServerAddresses()
{
    QSettings settings;
    settings.beginGroup(kUsedServerAddresses);
    QStringList retval = settings.value("main").toStringList();
    settings.endGroup();
    QString preconfigure_addr = seafApplet->readPreconfigureExpandedString(kPreconfigureServerAddr);
    if (!preconfigure_addr.isEmpty() && !retval.contains(preconfigure_addr)) {
        retval.push_back(preconfigure_addr);
    }
    if (!retval.contains(kDefaultServerAddr1)) {
        retval.push_back(kDefaultServerAddr1);
    }
    return retval;
}

void saveUsedServerAddresses(const QString &new_address)
{
    QSettings settings;
    settings.beginGroup(kUsedServerAddresses);
    QStringList list = settings.value("main").toStringList();
    // put the last used address to the front
    list.removeAll(new_address);
    list.insert(0, new_address);
    settings.setValue("main", list);
    settings.endGroup();
}

} // namespace

LoginDialog::LoginDialog(QWidget *parent) : QDialog(parent)
{
    setupUi(this);
    setWindowTitle(tr("Add an account"));
    setWindowIcon(QIcon(":/images/seafile.png"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    request_ = NULL;
    account_info_req_ = NULL;

    mStatusText->setText("");
    mLogo->setPixmap(QPixmap(":/images/seafile-32.png"));
    QString preconfigure_addr = seafApplet->readPreconfigureExpandedString(kPreconfigureServerAddr);
    if (seafApplet->readPreconfigureEntry(kPreconfigureServerAddrOnly).toBool() && !preconfigure_addr.isEmpty()) {
        mServerAddr->setMaxCount(1);
        mServerAddr->insertItem(0, preconfigure_addr);
        mServerAddr->setCurrentIndex(0);
        mServerAddr->setEditable(false);
    } else {
        mServerAddr->addItems(getUsedServerAddresses());
        mServerAddr->clearEditText();
    }
    mServerAddr->setAutoCompletion(false);

    QString computerName = seafApplet->settingsManager()->getComputerName();

    mComputerName->setText(computerName);

    connect(mSubmitBtn, SIGNAL(clicked()), this, SLOT(doLogin()));

    const QRect screen = QApplication::desktop()->screenGeometry();
    move(screen.center() - this->rect().center());

#ifdef HAVE_SHIBBOLETH_SUPPORT
    setupShibLoginLink();
#else
    mShibLoginLink->hide();
#endif
}

#ifdef HAVE_SHIBBOLETH_SUPPORT
void LoginDialog::setupShibLoginLink()
{
    QString txt = QString("<a style=\"color:#777\" href=\"#\">%1</a>").arg(tr("Shibboleth Login"));
    mShibLoginLink->setText(txt);
    connect(mShibLoginLink, SIGNAL(linkActivated(const QString&)),
            this, SLOT(loginWithShib()));
}
#endif // HAVE_SHIBBOLETH_SUPPORT

void LoginDialog::initFromAccount(const Account& account)
{
    setWindowTitle(tr("Re-login"));
    mTitle->setText(tr("Re-login"));
    mServerAddr->setMaxCount(1);
    mServerAddr->insertItem(0, account.serverUrl.toString());
    mServerAddr->setCurrentIndex(0);
    mServerAddr->setEditable(false);

    mUsername->setText(account.username);
    mPassword->setFocus(Qt::OtherFocusReason);
}

void LoginDialog::doLogin()
{
    if (!validateInputs()) {
        return;
    }
    saveUsedServerAddresses(url_.toString());

    mStatusText->setText(tr("Logging in..."));

    disableInputs();

    if (request_) {
        request_->deleteLater();
    }

    request_ = new LoginRequest(url_, username_, password_, computer_name_);
    if (!two_factor_auth_token_.isEmpty()) {
        request_->setHeader(kSeafileOTPHeader, two_factor_auth_token_);
    }

    connect(request_, SIGNAL(success(const QString&)),
            this, SLOT(loginSuccess(const QString&)));

    connect(request_, SIGNAL(failed(const ApiError&)),
            this, SLOT(loginFailed(const ApiError&)));

    request_->send();
}

void LoginDialog::disableInputs()
{
    mServerAddr->setEnabled(false);
    mUsername->setEnabled(false);
    mPassword->setEnabled(false);
    mSubmitBtn->setEnabled(false);
    mComputerName->setEnabled(false);
}

void LoginDialog::enableInputs()
{
    mServerAddr->setEnabled(true);
    mUsername->setEnabled(true);
    mPassword->setEnabled(true);
    mSubmitBtn->setEnabled(true);
    mComputerName->setEnabled(true);
}

void LoginDialog::onNetworkError(const QNetworkReply::NetworkError& error, const QString& error_string)
{
    showWarning(tr("Network Error:\n %1").arg(error_string));
    enableInputs();

    mStatusText->setText("");
}

void LoginDialog::onSslErrors(QNetworkReply* reply, const QList<QSslError>& errors)
{
    const QSslCertificate &cert = reply->sslConfiguration().peerCertificate();
    qDebug() << "\n= SslErrors =\n" << dumpSslErrors(errors);
    qDebug() << "\n= Certificate =\n" << dumpCertificate(cert);

    if (seafApplet->detailedYesOrNoBox(tr("<b>Warning:</b> The ssl certificate of this server is not trusted, proceed anyway?"),
                                   dumpSslErrors(errors) + dumpCertificate(cert),
                                   this,
                                   false))
        reply->ignoreSslErrors();
}

bool LoginDialog::validateInputs()
{
    QString serverAddr = mServerAddr->currentText();
    QString protocol;
    QUrl url;

    if (serverAddr.size() == 0) {
        showWarning(tr("Please enter the server address"));
        return false;
    } else {
        if (!serverAddr.startsWith("http://") && !serverAddr.startsWith("https://")) {
            showWarning(tr("%1 is not a valid server address").arg(serverAddr));
            return false;
        }

        url = QUrl(serverAddr, QUrl::StrictMode);
        if (!url.isValid()) {
            showWarning(tr("%1 is not a valid server address").arg(serverAddr));
            return false;
        }
    }

    QString email = mUsername->text();
    if (email.size() == 0) {
        showWarning(tr("Please enter the username"));
        return false;
    }

    if (mPassword->text().size() == 0) {
        showWarning(tr("Please enter the password"));
        return false;
    }

    QString computer_name = mComputerName->text().trimmed();
    if (computer_name.size() == 0) {
        showWarning(tr("Please enter the computer name"));
        return false;
    }

    url_ = url;
    username_ = mUsername->text();
    password_ = mPassword->text();
    computer_name_ = mComputerName->text();

    seafApplet->settingsManager()->setComputerName(computer_name_);

    return true;
}

void LoginDialog::loginSuccess(const QString& token)
{
    if (account_info_req_) {
        account_info_req_->deleteLater();
    }
    account_info_req_ =
        new FetchAccountInfoRequest(Account(url_, username_, token));
    connect(account_info_req_, SIGNAL(success(const AccountInfo&)), this,
            SLOT(onFetchAccountInfoSuccess(const AccountInfo&)));
    connect(account_info_req_, SIGNAL(failed(const ApiError&)), this,
            SLOT(onFetchAccountInfoFailed(const ApiError&)));
    account_info_req_->send();
}

void LoginDialog::onFetchAccountInfoFailed(const ApiError& error)
{
    loginFailed(error);
}

void LoginDialog::loginFailed(const ApiError& error)
{
    switch (error.type()) {
    case ApiError::SSL_ERROR:
        onSslErrors(error.sslReply(), error.sslErrors());
        break;
    case ApiError::NETWORK_ERROR:
        onNetworkError(error.networkError(), error.networkErrorString());
        break;
    case ApiError::HTTP_ERROR:
        onHttpError(error.httpErrorCode());
    default:
        // impossible
        break;
    }
}

void LoginDialog::onFetchAccountInfoSuccess(const AccountInfo& info)
{
    Account account = account_info_req_->account();
    // The user may use the username to login, but we need to store the email
    // to account database
    account.username = info.email;
    if (seafApplet->accountManager()->saveAccount(account) < 0) {
        showWarning(tr("Failed to save current account"));
    }
    else {
        seafApplet->accountManager()->updateAccountInfo(account, info);
        done(QDialog::Accepted);
    }
}

void LoginDialog::onHttpError(int code)
{
    const QNetworkReply* reply = request_->reply();
    if (reply->hasRawHeader(kSeafileOTPHeader) &&
        QString(reply->rawHeader(kSeafileOTPHeader)) == "required") {
        two_factor_auth_token_ = QInputDialog::getText(
            this,
            tr("Two Factor Authentication"),
            tr("Enter the two factor authentication token"),
            QLineEdit::Normal,
            "");
        if (!two_factor_auth_token_.isEmpty()) {
            doLogin();
            return;
        }
    } else {
        QString err_msg, reason;
        if (code == 400) {
            reason = tr("Incorrect email or password");
        } else if (code == 429) {
            reason = tr("Logging in too frequently, please wait a minute");
        } else if (code == 500) {
            reason = tr("Internal Server Error");
        }

        if (reason.length() > 0) {
            err_msg = tr("Failed to login: %1").arg(reason);
        } else {
            err_msg = tr("Failed to login");
        }

        showWarning(err_msg);
    }

    enableInputs();

    mStatusText->setText("");
}

void LoginDialog::showWarning(const QString& msg)
{
    seafApplet->warningBox(msg, this);
}

#ifdef HAVE_SHIBBOLETH_SUPPORT
void LoginDialog::loginWithShib()
{
    QString serverAddr = seafApplet->settingsManager()->getLastShibUrl();
    serverAddr = QInputDialog::getText(this, tr("Shibboleth Login"),
                                       tr("%1 Server Address").arg(getBrand()),
                                       QLineEdit::Normal,
                                       serverAddr);
    serverAddr = serverAddr.trimmed();
    if (serverAddr.isEmpty()) {
        return;
    }

    if (!serverAddr.startsWith("http://") && !serverAddr.startsWith("https://")) {
        showWarning(tr("%1 is not a valid server address").arg(serverAddr));
        return;
    }

    QUrl url = QUrl(serverAddr, QUrl::StrictMode);
    if (!url.isValid()) {
        showWarning(tr("%1 is not a valid server address").arg(serverAddr));
        return;
    }

    seafApplet->settingsManager()->setLastShibUrl(serverAddr);

    ShibLoginDialog shib_dialog(url, mComputerName->text(), this);
    if (shib_dialog.exec() == QDialog::Accepted) {
        accept();
    }
}
#endif // HAVE_SHIBBOLETH_SUPPORT
