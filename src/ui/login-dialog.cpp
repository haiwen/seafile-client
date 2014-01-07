#include <QtGui>
#include <QtNetwork>

#include "account-mgr.h"
#include "seafile-applet.h"
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

    request_ = NULL;

    mStatusText->setText("");
    mLogo->setPixmap(QPixmap(":/images/seafile-32.png"));
    mServerAddr->addItem(kDefaultServerAddr1);
    mServerAddr->addItem(kDefaultServerAddr2);
    mServerAddr->clearEditText();
    mServerAddr->setAutoCompletion(false);

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

    request_ = new LoginRequest(url_, username_, password_);
    request_->setIgnoreSslErrors(false);

    connect(request_, SIGNAL(success(const QString&)),
            this, SLOT(loginSuccess(const QString&)));

    connect(request_, SIGNAL(failed(int)),
            this, SLOT(loginFailed(int)));

    connect(request_, SIGNAL(networkError(const QNetworkReply::NetworkError&, const QString&)),
            this, SLOT(onNetworkError(const QNetworkReply::NetworkError&, const QString&)));

    connect(request_, SIGNAL(sslErrors(QNetworkReply*, const QList<QSslError>&)),
            this, SLOT(onSslErrors(QNetworkReply*, const QList<QSslError>&)));

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
}

void LoginDialog::onSslErrors(QNetworkReply* reply, const QList<QSslError>& errors)
{
    QString question = tr("<b>Warning:</b> The ssl certificate of this server is not trusted, proceed anyway?");
    if (QMessageBox::question(this,
                              tr(SEAFILE_CLIENT_BRAND),
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

    url_ = url;
    username_ = mUsername->text();
    password_ = mPassword->text();

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

void LoginDialog::loginFailed(int code)
{
    QString err_msg, reason;
    if (code == 400) {
        reason = tr("Incorrect email or password");
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
