#include <QtGui>

#include "account-mgr.h"
#include "seafile-applet.h"
#include "configurator.h"
#include "api/requests.h"
#include "rpc/rpc-client.h"
#include "create-repo-dialog.h"

CreateRepoDialog::CreateRepoDialog(const Account& account,
                                   const QString& worktree,
                                   QWidget *parent)
    : QDialog(parent),
      request_(NULL),
      path_(worktree),
      account_(account)
{
    setupUi(this);
    setWindowTitle(tr("Create a library"));

    mStatusText->setText("");

    if (!worktree.isEmpty()) {
        mDirectory->setText(worktree);
    }

    connect(mChooseDirBtn, SIGNAL(clicked()), this, SLOT(chooseDirAction()));
    connect(mOkBtn, SIGNAL(clicked()), this, SLOT(createAction()));
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
    if (dir.isEmpty())
        return;
    mDirectory->setText(dir);
    if (mName->text().isEmpty()) {
        QDir d(dir);
        mName->setText(d.dirName());
    }
}

void CreateRepoDialog::setAllInputsEnabled(bool enabled)
{
    mOkBtn->setEnabled(enabled);
    mChooseDirBtn->setEnabled(enabled);
    mDirectory->setEnabled(enabled);
    mName->setEnabled(enabled);
    mDesc->setEnabled(enabled);
    mEncrypteCheckBox->setEnabled(enabled);

    bool password_enabled = (mEncrypteCheckBox->checkState() == Qt::Checked) && enabled;
    mPassword->setEnabled(password_enabled);
    mPasswordAgain->setEnabled(password_enabled);
}

void CreateRepoDialog::createAction()
{
    if (!validateInputs()) {
        return;
    }
    mStatusText->setText(tr("Creating..."));

    setAllInputsEnabled(false);

    if (request_) {
        delete request_;
    }
    request_ = new CreateRepoRequest(account_, name_, desc_, passwd_);

    connect(request_, SIGNAL(success(const RepoDownloadInfo&)),
            this, SLOT(createSuccess(const RepoDownloadInfo&)));

    connect(request_, SIGNAL(failed(int)),
            this, SLOT(createFailed(int)));

    request_->send();
}

bool CreateRepoDialog::validateInputs()
{
    QString path;
    QString passwd;
    QString passwdAgain;
    bool encrypted;

    path = mDirectory->text();
    if (path.isEmpty()) {
        QMessageBox::warning(this, tr(SEAFILE_CLIENT_BRAND),
                             tr("Please choose the directory to sync"),
                             QMessageBox::Ok);
        return false;
    }
    if (!QDir(path).exists()) {
        QMessageBox::warning(this, tr(SEAFILE_CLIENT_BRAND),
                             tr("The folder %1 does not exist").arg(path),
                             QMessageBox::Ok);
        return false;
    }

    if (mName->text().isEmpty()) {
         QMessageBox::warning(this, tr(SEAFILE_CLIENT_BRAND),
                             tr("Please enter the name"),
                             QMessageBox::Ok);
        return false;
    }
    if (mDesc->toPlainText().isEmpty()) {
         QMessageBox::warning(this, tr(SEAFILE_CLIENT_BRAND),
                             tr("Please enter the description"),
                             QMessageBox::Ok);
        return false;
    }
    encrypted = (mEncrypteCheckBox->checkState() == Qt::Checked) ? true : false;
    if (encrypted) {
         if (mPassword->text().isEmpty() || mPasswordAgain->text().isEmpty()) {
             QMessageBox::warning(this, tr(SEAFILE_CLIENT_BRAND),
                                  tr("Please enter the password"),
                                  QMessageBox::Ok);
             return false;
         }
         passwd = mPassword->text();
         passwdAgain = mPasswordAgain->text();
         if (passwd != passwdAgain) {
             QMessageBox::warning(this, tr(SEAFILE_CLIENT_BRAND),
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

void CreateRepoDialog::createSuccess(const RepoDownloadInfo& info)
{
    QString error;

    int ret = seafApplet->rpcClient()->cloneRepo(
        info.repo_id,
        info.relay_id,
        info.repo_name,
        path_,
        info.token,
        passwd_,
        info.magic,
        info.relay_addr,
        info.relay_port,
        info.email,
        info.random_key,
        info.enc_version,
        &error);

    if (ret < 0) {
        QMessageBox::warning(this, tr(SEAFILE_CLIENT_BRAND),
                             tr("Failed to add download task:\n %1").arg(error),
                             QMessageBox::Ok);
        setAllInputsEnabled(true);
    } else {
        done(QDialog::Accepted);
    }
}

void CreateRepoDialog::createFailed(int /* code */)
{
    mStatusText->setText("");

    QMessageBox::warning(this, tr(SEAFILE_CLIENT_BRAND),
                         tr("Failed to create library on the server"),
                         QMessageBox::Ok);

    setAllInputsEnabled(true);
}

