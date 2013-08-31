#include <QtGui>

#include "account-mgr.h"
#include "seafile-applet.h"
#include "configurator.h"
#include "api/requests.h"
#include "rpc/rpc-client.h"
#include "create-repo-dialog.h"

CreateRepoDialog::CreateRepoDialog(const Account& account, QWidget *parent)
    : QDialog(parent),
      request_(NULL),
      account_(account)
{
    setupUi(this);
    setWindowTitle(tr("Create a repo"));

    mStatusText->setText("");

    connect(mChooseDirBtn, SIGNAL(clicked()), this, SLOT(chooseDirAction()));
    connect(mCreateBtn, SIGNAL(clicked()), this, SLOT(createAction()));
    connect(mEncrypteCheckBox, SIGNAL(stateChanged(int)), this, SLOT(encryptedChanged(int)));
}

CreateRepoDialog::~CreateRepoDialog()
{
    if (request_)
        delete request_;
}

void CreateRepoDialog::chooseDirAction()
{
    const QString &wt = seafApplet->configurator()->worktreeDir();
    QString dir = QFileDialog::getExistingDirectory(this, tr("Please choose a directory"),
                                                        wt.toUtf8().data(),
                                                        QFileDialog::ShowDirsOnly
                                                        | QFileDialog::DontResolveSymlinks);

    qDebug() << "Choose directory:" << dir;
    if (dir.isEmpty())
        return;
    mDirectory->setText(dir);
    if (mName->text().isEmpty()) {
        QDir d(dir);
        mName->setText(d.dirName());
    }
}

void CreateRepoDialog::encryptedChanged(int state)
{
    if (state == Qt::Checked) {
        mPassword->setEnabled(true);
        mPasswordAgain->setEnabled(true);
    } else {
        mPassword->setEnabled(false);
        mPasswordAgain->setEnabled(false);
    }
}

void CreateRepoDialog::createAction()
{
    if (!validateInputs()) {
        return;
    }
    mStatusText->setText(tr("Creating repo..."));

    mCreateBtn->setEnabled(false);
    mChooseDirBtn->setEnabled(false);
    mDirectory->setEnabled(false);
    mName->setEnabled(false);
    mDesc->setEnabled(false);
    mEncrypteCheckBox->setEnabled(false);
    if (mEncrypteCheckBox->checkState() == Qt::Checked) {
        mPassword->setEnabled(false);
        mPasswordAgain->setEnabled(false);
    }

    if (request_) {
        delete request_;
    }
    qDebug() << name_ << desc_ << passwd_;
    request_ = new CreateRepoRequest(account_, name_, desc_, passwd_);

    connect(request_, SIGNAL(success(const QMap<QString, QString> &)),
            this, SLOT(createSuccess(const QMap<QString, QString> &)));

    connect(request_, SIGNAL(failed(int)),
            this, SLOT(createFailed(int)));

    request_->send();
}

bool CreateRepoDialog::validateInputs()
{
    QString passwd;
    QString passwdAgain;
    bool encrypted;

    if (mDirectory->text().isEmpty()) {
        QMessageBox::warning(this, tr("Seafile"),
                             tr("Please choose the drectory to sync"),
                             QMessageBox::Ok);
        return false;
    }
    QDir dir(mDirectory->text());
    if (!dir.exists()) {
        QMessageBox::warning(this, tr("Seafile"),
                             tr("Directory donot exist"),
                             QMessageBox::Ok);
        return false;
    }

    if (mName->text().isEmpty()) {
         QMessageBox::warning(this, tr("Seafile"),
                             tr("Please enter the name"),
                             QMessageBox::Ok);
        return false;
    }
    if (mDesc->toPlainText().isEmpty()) {
         QMessageBox::warning(this, tr("Seafile"),
                             tr("Please enter the description"),
                             QMessageBox::Ok);
        return false;
    }
    encrypted = (mEncrypteCheckBox->checkState() == Qt::Checked) ? true : false;
    if (encrypted) {
         if (mPassword->text().isEmpty() || mPasswordAgain->text().isEmpty()) {
             QMessageBox::warning(this, tr("Seafile"),
                                  tr("Please enter the password"),
                                  QMessageBox::Ok);
             return false;
         }
         passwd = mPassword->text();
         passwdAgain = mPasswordAgain->text();
         if (passwd != passwdAgain) {
             QMessageBox::warning(this, tr("Seafile"),
                                  tr("Passwords don't match"),
                                  QMessageBox::Ok);
             return false;
         }
         passwd_ = passwd;
    } else {
        passwd_ = QString::null;
    }
    name_ = mName->text();
    desc_ = mDesc->toPlainText();
    path_ = mDirectory->text();
    return true;
}

void CreateRepoDialog::createSuccess(const QMap<QString, QString> &dict)
{
    qDebug() << __func__ << ":" << dict["repo_id"];
    connect(seafApplet->rpcClient(),
            SIGNAL(cloneRepoSignal(QString &, bool)),
            this, SLOT(cloneRepoRequestFinished(QString &, bool)));
    seafApplet->rpcClient()->cloneRepo(dict["repo_id"], dict["relay_id"], dict["repo_name"], path_, dict["token"], passwd_, dict["magic"], dict["relay_addr"], dict["relay_port"], dict["email"]);
}

void CreateRepoDialog::createFailed(int code)
{
    mStatusText->setText("");

    qDebug("%s", __func__);
    mCreateBtn->setEnabled(true);
    mChooseDirBtn->setEnabled(true);
    mDirectory->setEnabled(true);
    mName->setEnabled(true);
    mDesc->setEnabled(true);
    mEncrypteCheckBox->setEnabled(true);
    if (mEncrypteCheckBox->checkState() == Qt::Checked) {
        mPassword->setEnabled(true);
        mPasswordAgain->setEnabled(true);
    }
}

void CreateRepoDialog::cloneRepoRequestFinished(QString &repoId, bool result)
{
    qDebug() << __func__ << ":" << result;
    disconnect(seafApplet->rpcClient(),
               SIGNAL(cloneRepoSignal(QString &, bool)),
               this, SLOT(cloneRepoRequestFinished(QString &, bool)));
    if (result)
        done(QDialog::Accepted);
    else {
        qDebug() << "Failed to sync the directory";
    }
}
