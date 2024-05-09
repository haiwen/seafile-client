#include <QtGlobal>

#include <QtWidgets>
#include <QUuid>

#include "utils/utils.h"
#include "account-mgr.h"
#include "seafile-applet.h"
#include "configurator.h"
#include "repo-service.h"
#include "api/requests.h"
#include "api/api-error.h"
#include "rpc/rpc-client.h"
#include "ui/create-repo-dialog.h"
#include "ui/repos-tab.h"

#ifdef Q_OS_WIN32
#include "utils/utils-win.h"
#endif

CreateRepoDialog::CreateRepoDialog(const Account& account,
                                   const QString& worktree,
                                   ReposTab *repos_tab,
                                   QWidget *parent)
    : QDialog(parent),
      path_(worktree),
      request_(NULL),
      account_(account),
      repos_tab_(repos_tab)
{
    setupUi(this);
    setWindowTitle(tr("Create a library"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowIcon(QIcon(":/images/seafile.png"));

#if defined(Q_OS_MAC)
    layout()->setContentsMargins(6, 6, 6, 6);
    layout()->setSpacing(5);
#endif

    mStatusText->setText("");

    mDirectory->setText(worktree);
    mName->setText(QDir(worktree).dirName());

    connect(mChooseDirBtn, SIGNAL(clicked()), this, SLOT(chooseDirAction()));
    connect(mOkBtn, SIGNAL(clicked()), this, SLOT(createAction()));

    const QRect screen = getScreenSize(0);

    move(screen.center() - this->rect().center());

    mTipLabel->setText("(" + tr("end-to-end encryption") + ")");
}

CreateRepoDialog::~CreateRepoDialog()
{
    if (request_) {
        request_->deleteLater();
        request_ = nullptr;
    }
}

void CreateRepoDialog::chooseDirAction()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Please choose a directory"),
                                                    mDirectory->text(),
                                                    QFileDialog::ShowDirsOnly
                                                    | QFileDialog::DontResolveSymlinks);
    if (dir.isEmpty())
        return;
    mDirectory->setText(dir);
    QDir d(dir);
    mName->setText(d.dirName());
}

void CreateRepoDialog::setAllInputsEnabled(bool enabled)
{
    mOkBtn->setEnabled(enabled);
    mChooseDirBtn->setEnabled(enabled);
    mDirectory->setEnabled(enabled);
    mName->setEnabled(enabled);
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
#ifdef Q_OS_WIN32
    if (utils::win::isNetworkDevice(path_)) {
        bool ok = seafApplet->yesOrCancelBox(
            tr("File changes on network drives may not be synced automatically. You can set sync intervals to enable periodic sync. Do you want to sync with this folder?"),
            this, true);
        if (!ok) {
            return;
        }
    }
#endif // Q_OS_WIN32

    mStatusText->setText(tr("Creating..."));

    setAllInputsEnabled(false);

    if (request_) {
        request_->deleteLater();
        request_ = nullptr;
    }

    if (!passwd_.isEmpty() && account_.isAtLeastVersion(4, 4, 0)) {
        // TODO: check server version is at least 4.3.x ?
        QString repo_id = QUuid::createUuid().toString().mid(1, 36);
        QString magic, random_key, salt;

        int enc_version = seafApplet->accountManager()->currentAccount().getEncryptedLibraryVersion();

        if (enc_version < 2) {
            seafApplet->warningBox(tr("Creating a library with encryption version less than 2 is not supported"), this);
            return;
        }

        if (seafApplet->rpcClient()->generateMagicAndRandomKey(
                enc_version, repo_id, passwd_, &magic, &random_key, &salt) < 0) {
            seafApplet->warningBox(tr("Failed to generate encryption key for this library"), this);
            return;
        }

        if (enc_version == 3 || enc_version == 4) {
            request_ = new CreateRepoRequest(
                account_, name_, name_, enc_version, repo_id, magic, random_key, salt);
        } else {
            request_ = new CreateRepoRequest(
                account_, name_, name_, enc_version, repo_id, magic, random_key);
        }
    } else {
        request_ = new CreateRepoRequest(account_, name_, name_, passwd_);
    }

    connect(request_, SIGNAL(success(const RepoDownloadInfo&)),
            this, SLOT(createSuccess(const RepoDownloadInfo&)));

    connect(request_, SIGNAL(failed(const ApiError&)),
            this, SLOT(createFailed(const ApiError&)));

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
        seafApplet->warningBox(tr("Please choose the directory to sync"), this);
        return false;
    }
    if (!QDir(path).exists()) {
        seafApplet->warningBox(tr("The folder %1 does not exist").arg(path), this);
        return false;
    }

    if (mName->text().trimmed().isEmpty()) {
        seafApplet->warningBox(tr("Please enter the name"), this);
        return false;
    }

    encrypted = mEncrypteCheckBox->checkState() == Qt::Checked;
    if (encrypted) {
        if (mPassword->text().trimmed().isEmpty() ||
            mPasswordAgain->text().trimmed().isEmpty()) {
            seafApplet->warningBox(tr("Please enter the password"), this);
            return false;
        }

        passwd = mPassword->text().trimmed();
        passwdAgain = mPasswordAgain->text().trimmed();
        if (passwd != passwdAgain) {
            seafApplet->warningBox(tr("Passwords don't match"), this);
            return false;
        }
        passwd_ = passwd;
    } else {
        passwd_ = QString();
    }

    QString error;
    if (seafApplet->rpcClient()->checkPathForClone(path, &error) < 0) {
        if (error.isEmpty()) {
            error = tr("Unknown error");
        }
        seafApplet->warningBox(translateErrorMsg(error), this);
        return false;
    }

    name_ = mName->text().trimmed();
    path_ = mDirectory->text();
    return true;
}

void CreateRepoDialog::createSuccess(const RepoDownloadInfo& info)
{
    QString error;

    int ret = seafApplet->rpcClient()->cloneRepo(
        info.repo_id,
        info.repo_version,
        info.repo_name,
        path_,
        info.token,
        passwd_,
        info.magic,
        info.email,
        info.random_key,
        info.enc_version,
        info.more_info,
        &error);

    if (ret < 0) {
        error = translateErrorMsg(error);
        seafApplet->warningBox(tr("Failed to add download task:\n %1").arg(error), this);
        setAllInputsEnabled(true);
    } else {
        repos_tab_->refresh();
        done(QDialog::Accepted);
    }
}

void CreateRepoDialog::createFailed(const ApiError& error)
{
    mStatusText->setText("");

    QString msg = tr("Failed to create library on the server:\n%1").arg(error.toString());

    seafApplet->warningBox(msg, this);

    setAllInputsEnabled(true);
}

QString CreateRepoDialog::translateErrorMsg(const QString& error)
{
    if (error == "Worktree conflicts system path") {
        return QObject::tr("The path \"%1\" conflicts with system path").arg(path_);
    } else if (error == "Worktree conflicts existing repo") {
        return QObject::tr("The path \"%1\" conflicts with an existing library").arg(path_);
    }
    return error;
}
