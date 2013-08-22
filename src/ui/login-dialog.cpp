#include <QtGui>

#include "account-mgr.h"
#include "seafile-applet.h"
#include "api/requests.h"
#include "login-dialog.h"

LoginDialog::LoginDialog(QWidget *parent) : QDialog(parent)
{
    setupUi(this);
    setWindowTitle(tr("Add an account"));

    request_ = NULL;

    mStatusText->setText("");

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

    connect(request_, SIGNAL(success(const QString&)),
            this, SLOT(loginSuccess(const QString&)));

    connect(request_, SIGNAL(failed(int)),
            this, SLOT(loginFailed()));

    request_->send();
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
        protocol = mHttpsCheckBox->isChecked() ? "https://" : "http://";
        url = QUrl(protocol + serverAddr, QUrl::StrictMode);
        qDebug("url is %s\n", url.toString().toUtf8().data());
        if (!url.isValid()) {
            QMessageBox::warning(this, tr("Seafile"),
                                 tr("%1 is not a valid server address")
                                 .arg(url.toString()),
                                 QMessageBox::Ok);
            return false;
        }
    }

    if (mUsername->text().size() == 0) {
        QMessageBox::warning(this, tr("Seafile"),
                             tr("Please enter the username"),
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

void LoginDialog::loginFailed()
{
    QMessageBox::warning(this, tr("Seafile"),
                         tr("Failed to login"),
                         QMessageBox::Ok);

    mSubmitBtn->setEnabled(true);
    mServerAddr->setEnabled(true);
    mUsername->setEnabled(true);
    mPassword->setEnabled(true);

    mStatusText->setText("");
}
