#include <QtGui>

#include "login-dialog.h"
#include "api-client.h"
#include "account-mgr.h"

LoginDialog::LoginDialog(QWidget *parent) : QDialog(parent)
{
    setupUi(this);
    setWindowTitle(tr("Add an account"));

    mStatusText->setText("");

    connect(mSubmitBtn, SIGNAL(clicked()), this, SLOT(doLogin()));

    SeafileApiClient *conn = SeafileApiClient::instance();
    connect(conn, SIGNAL(accountLoginSuccess(const Account&)),
            this, SLOT(loginSuccess(const Account&)));

    connect(conn, SIGNAL(accountLoginFailed()), this, SLOT(loginFailed()));
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

    SeafileApiClient *conn = SeafileApiClient::instance();
    conn->accountLogin(url_, username_, password_);
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

void LoginDialog::loginSuccess(const Account& account)
{
    AccountManager *mgr = AccountManager::instance();
    if (mgr->saveAccount(account) < 0) {
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
