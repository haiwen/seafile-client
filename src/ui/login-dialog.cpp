#include <QtGui>
#include <QtNetwork>

#include "account-mgr.h"
#include "seafile-applet.h"
#include "api/requests.h"
#include "login-dialog.h"

LoginDialog::LoginDialog(QWidget *parent) : QDialog(parent)
{
    setupUi(this);
    setWindowTitle(tr("Add an account"));
    setWindowIcon(QIcon(":/images/seafile.png"));

    request_ = NULL;

    mStatusText->setText("");
    mLogo->setPixmap(QPixmap(":/images/seafile-32.png"));

    connect(mSubmitBtn, SIGNAL(clicked()), this, SLOT(doLogin()));
}

void LoginDialog::doLogin()
{
    if (!validateInputs()) {
        return;
    }
    mStatusText->setText(tr("Logging in..."));

    mServerAddr->setEnabled(false);
    mUsername->setEnabled(false);
    mPassword->setEnabled(false);
    mSubmitBtn->setEnabled(false);

    if (request_) {
        delete request_;
    }

    request_ = new LoginRequest(url_, username_, password_);
    request_->setIgnoreSslErrors(false);

    connect(request_, SIGNAL(success(const QString&)),
            this, SLOT(loginSuccess(const QString&)));

    connect(request_, SIGNAL(failed(int)),
            this, SLOT(loginFailed(int)));

    connect(request_, SIGNAL(sslErrors(QNetworkReply*, const QList<QSslError>&)),
            this, SLOT(onSslErrors(QNetworkReply*, const QList<QSslError>&)));

    request_->send();
}

void LoginDialog::onSslErrors(QNetworkReply* reply, const QList<QSslError>& errors)
{
    QString question = tr("<b>Warning:</b> The ssl certificate of this server is not trusted, proceed anyway?");
    if (QMessageBox::question(this,
                              tr("Seafile"),
                              question,
                              QMessageBox::Yes | QMessageBox::No,
                              QMessageBox::No) == QMessageBox::Yes) {
        reply->ignoreSslErrors();
    }
}

bool LoginDialog::validateInputs()
{
    QString serverAddr = mServerAddr->text();
    QString protocol;
    QUrl url;

    if (serverAddr.size() == 0) {
        QMessageBox::warning(this, tr("Seafile"),
                             tr("Please enter the server address"),
                             QMessageBox::Ok);
        return false;
    } else {
        if (!serverAddr.startsWith("http://") && !serverAddr.startsWith("https://")) {
            QMessageBox::warning(this, tr("Seafile"),
                                 tr("%1 is not a valid server address")
                                 .arg(serverAddr),
                                 QMessageBox::Ok);
            return false;
        }

        url = QUrl(serverAddr, QUrl::StrictMode);
        // qDebug("url is %s\n", url.toString().toUtf8().data());
        if (!url.isValid()) {
            QMessageBox::warning(this, tr("Seafile"),
                                 tr("%1 is not a valid server address")
                                 .arg(serverAddr),
                                 QMessageBox::Ok);
            return false;
        }
    }

    QString email = mUsername->text();
    if (email.size() == 0) {
        QMessageBox::warning(this, tr("Seafile"),
                             tr("Please enter the username"),
                             QMessageBox::Ok);
        return false;
    } else if (!email.contains("@")) {
        QMessageBox::warning(this, tr("Seafile"),
                             tr("%1 is not a valid email")
                             .arg(email),
                             QMessageBox::Ok);
        return false;
    }

    if (mPassword->text().size() == 0) {
        QMessageBox::warning(this, tr("Seafile"),
                             tr("Please enter the password"),
                             QMessageBox::Ok);
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
        QMessageBox::warning(this, tr("Seafile"),
                             tr("Internal Error"),
                             QMessageBox::Ok);
    } else {
        done(QDialog::Accepted);
    }
}

void LoginDialog::loginFailed(int code)
{
    QString err_msg, reason;
    if (code == 400) {
        reason = tr("Incorrect email or password");
    } else if (code != 0) {
        reason = tr("error code %1").arg(code);
    }

    if (reason.length() > 0) {
        err_msg = tr("Failed to login: %1").arg(reason);
    } else {
        err_msg = tr("Failed to login");
    }

    QMessageBox::warning(this, tr("Seafile"),
                         err_msg,
                         QMessageBox::Ok);

    mSubmitBtn->setEnabled(true);
    mServerAddr->setEnabled(true);
    mUsername->setEnabled(true);
    mPassword->setEnabled(true);

    mStatusText->setText("");
}
