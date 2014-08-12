#include <QtGlobal>

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
#include <QtWidgets>
#else
#include <QtGui>
#endif
#include <QtNetwork>

#include "settings-mgr.h"
#include "account-mgr.h"
#include "seafile-applet.h"
#include "api/api-error.h"
#include "api/requests.h"
#include "login-dialog.h"

namespace {

const QString kDefaultServerAddr1 = "https://seacloud.cc";
const QString kDefaultServerAddr2 = "https://cloud.seafile.com";

} // namespace

LoginDialog::LoginDialog(QWidget *parent) : QDialog(parent)
{
    setupUi(this);
    setWindowTitle(tr("Add an account"));
    setWindowIcon(QIcon(":/images/seafile.png"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    request_ = NULL;

    mStatusText->setText("");
    mLogo->setPixmap(QPixmap(":/images/seafile-32.png"));
    mServerAddr->addItem(kDefaultServerAddr1);
    mServerAddr->addItem(kDefaultServerAddr2);
    mServerAddr->clearEditText();
    mServerAddr->setAutoCompletion(false);

    QString computerName = seafApplet->settingsManager()->getComputerName();

    mComputerName->setText(computerName);

    connect(mSubmitBtn, SIGNAL(clicked()), this, SLOT(doLogin()));

    const QRect screen = QApplication::desktop()->screenGeometry();
    move(screen.center() - this->rect().center());
}

void LoginDialog::doLogin()
{
    if (!validateInputs()) {
        return;
    }
    mStatusText->setText(tr("Logging in..."));

    disableInputs();

    if (request_) {
        delete request_;
    }

    request_ = new LoginRequest(url_, username_, password_, computer_name_);
    request_->setIgnoreSslErrors(false);

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
}

void LoginDialog::enableInputs()
{
    mSubmitBtn->setEnabled(true);
    mServerAddr->setEnabled(true);
    mUsername->setEnabled(true);
    mPassword->setEnabled(true);
}

void LoginDialog::onNetworkError(const QNetworkReply::NetworkError& error, const QString& error_string)
{
    showWarning(tr("Network Error:\n %1").arg(error_string));
    enableInputs();

    mStatusText->setText("");
}

void LoginDialog::onSslErrors(QNetworkReply* reply, const QList<QSslError>& errors)
{
    QString question = tr("<b>Warning:</b> The ssl certificate of this server is not trusted, proceed anyway?");
    if (QMessageBox::question(this,
                              getBrand(),
                              question,
                              QMessageBox::Yes | QMessageBox::No,
                              QMessageBox::No) == QMessageBox::Yes) {
        reply->ignoreSslErrors();
    }
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
    Account account(url_, username_, token);
    if (seafApplet->accountManager()->saveAccount(account) < 0) {
        showWarning(tr("Failed to save current account"));
    } else {
        done(QDialog::Accepted);
    }
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

void LoginDialog::onHttpError(int code)
{
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

    enableInputs();

    mStatusText->setText("");
}

void LoginDialog::showWarning(const QString& msg)
{
    seafApplet->warningBox(msg, this);
}
