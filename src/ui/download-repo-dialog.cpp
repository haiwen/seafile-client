#include <QtGlobal>

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
#include <QtWidgets>
#else
#include <QtGui>
#endif
#include <QDirIterator>

#include <jansson.h>

#include "account-mgr.h"
#include "utils/utils.h"
#include "seafile-applet.h"
#include "rpc/rpc-client.h"
#include "configurator.h"
#include "api/requests.h"
#include "api/api-error.h"
#include "api/server-repo.h"
#include "repo-service.h"
#include "download-repo-dialog.h"

namespace {
bool isPathConflictWithExistingRepo(const QString &path, QString *repo_name) {
    RepoService::instance()->refreshLocalRepoList();
    const std::vector<LocalRepo> & repos = RepoService::instance()->localRepos();
    for (unsigned i = 0; i < repos.size(); ++i) {
        if (repos[i].worktree == path) {
            *repo_name = repos[i].name;
            return true;
        }
    }
    return false;
}
} // anonymous namespace

DownloadRepoDialog::DownloadRepoDialog(const Account& account,
                                       const ServerRepo& repo,
                                       QWidget *parent)
    : QDialog(parent),
      repo_(repo),
      account_(account)
{
    setupUi(this);
    if (!repo.isSubfolder()) {
        setWindowTitle(tr("Sync library \"%1\"").arg(repo_.name));
        mDirectory->setPlaceholderText(tr("Sync this library to:"));
    }
    else {
        setWindowTitle(tr("Sync folder \"%1\"").arg(repo.parent_path));
        mDirectory->setPlaceholderText(tr("Sync this folder to:"));
    }
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    mRepoIcon->setPixmap(repo.getPixmap());
    mRepoName->setText(repo_.name);
    mOperationText->setText(tr("Sync to folder:"));
    mMergeHint->setText(tr("If a sub-folder with same name exists, its contents will be merged."));

    if (repo_.encrypted) {
        mPassword->setVisible(true);
        mPasswordLabel->setVisible(true);
    } else {
        mPassword->setVisible(false);
        mPasswordLabel->setVisible(false);
    }

    int height = 250;
#if defined(Q_OS_MAC)
    layout()->setContentsMargins(8, 9, 9, 5);
    layout()->setSpacing(6);
    verticalLayout_3->setSpacing(6);
#endif
    if (repo.encrypted) {
        height += 100;
    }
    setMinimumHeight(height);
    setMaximumHeight(height);

    setDirectoryText(seafApplet->configurator()->worktreeDir());

    connect(mChooseDirBtn, SIGNAL(clicked()), this, SLOT(chooseDirAction()));
    connect(mOkBtn, SIGNAL(clicked()), this, SLOT(onOkBtnClicked()));
}

void DownloadRepoDialog::setDirectoryText(const QString& path)
{
    QString text = path;
    if (text.endsWith("/")) {
        text.resize(text.size() - 1);
    }

    mDirectory->setText(text);
}

void DownloadRepoDialog::chooseDirAction()
{
    const QString &wt = seafApplet->configurator()->worktreeDir();
    QString dir = QFileDialog::getExistingDirectory(this, tr("Please choose a folder"),
                                                    wt.toUtf8().data(),
                                                    QFileDialog::ShowDirsOnly
                                                    | QFileDialog::DontResolveSymlinks);
    if (dir.isEmpty())
        return;
    setDirectoryText(dir);
}

void DownloadRepoDialog::onOkBtnClicked()
{
    if (!validateInputs()) {
        return;
    }

    setAllInputsEnabled(false);

    DownloadRepoRequest *req = new DownloadRepoRequest(account_, repo_.id, repo_.readonly);
    connect(req, SIGNAL(success(const RepoDownloadInfo&)),
            this, SLOT(onDownloadRepoRequestSuccess(const RepoDownloadInfo&)));
    connect(req, SIGNAL(failed(const ApiError&)),
            this, SLOT(onDownloadRepoRequestFailed(const ApiError&)));
    req->send();
}

bool DownloadRepoDialog::validateInputs()
{
    QString password;
    setDirectoryText(mDirectory->text().trimmed());
    if (mDirectory->text().isEmpty()) {
        QMessageBox::warning(this, getBrand(),
                             tr("Please choose the folder to sync."),
                             QMessageBox::Ok);
        return false;
    }
    sync_with_existing_ = false;
    QString path = QDir(mDirectory->text()).absoluteFilePath(repo_.name);
    QFileInfo fileinfo = QFileInfo(path);
    if (fileinfo.exists()) {
        // exist and but not a directory ?
        if (!fileinfo.isDir()) {
            QMessageBox::warning(this, getBrand(),
                                 tr("Conflicting with existing file \"%1\", please choose a different folder.").arg(path),
                                 QMessageBox::Ok);
            return false;
        }
        // exist and but conflicting?
        QString repo_name;
        if (isPathConflictWithExistingRepo(path, &repo_name)) {
            QMessageBox::warning(this, getBrand(),
                                 tr("Conflicting with existing library \"%1\", please choose a different folder.").arg(repo_name),
                                 QMessageBox::Ok);
            return false;
        }
        sync_with_existing_ = true;
        int ret = QMessageBox::question(
            this, getBrand(), tr("Are you sure to sync with the existing folder \"%1\"?")
                                  .arg(path),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (ret & QMessageBox::No)
            return false;
    }
    if (repo_.encrypted) {
        mPassword->setText(mPassword->text().trimmed());
        if (mPassword->text().isEmpty()) {
            QMessageBox::warning(this, getBrand(),
                                 tr("Please enter the password"),
                                 QMessageBox::Ok);
            return false;
        }
    }

    password = mPassword->text();
    return true;
}

void DownloadRepoDialog::setAllInputsEnabled(bool enabled)
{
    mDirectory->setEnabled(enabled);
    mChooseDirBtn->setEnabled(enabled);
    mPassword->setEnabled(enabled);
    mOkBtn->setEnabled(enabled);
}

void DownloadRepoDialog::onDownloadRepoRequestSuccess(const RepoDownloadInfo& info)
{
    QString worktree = mDirectory->text();
    QString password = repo_.encrypted ? mPassword->text() : QString();
    int ret = 0;
    QString error;

    if (sync_with_existing_) {
        worktree = QDir(worktree).absoluteFilePath(repo_.name);
        ret = seafApplet->rpcClient()->cloneRepo(info.repo_id, info.repo_version,
                                                 info.relay_id,
                                                 repo_.name, worktree,
                                                 info.token, password,
                                                 info.magic, info.relay_addr,
                                                 info.relay_port, info.email,
                                                 info.random_key, info.enc_version,
                                                 info.more_info,
                                                 &error);
    } else {
        ret = seafApplet->rpcClient()->downloadRepo(info.repo_id, info.repo_version,
                                                    info.relay_id,
                                                    repo_.name, worktree,
                                                    info.token, password,
                                                    info.magic, info.relay_addr,
                                                    info.relay_port, info.email,
                                                    info.random_key, info.enc_version,
                                                    info.more_info,
                                                    &error);
    }

    if (ret < 0) {
        QMessageBox::warning(this, getBrand(),
                             tr("Failed to add download task:\n %1").arg(error),
                             QMessageBox::Ok);
        setAllInputsEnabled(true);
    } else {
        done(QDialog::Accepted);
    }
}


void DownloadRepoDialog::onDownloadRepoRequestFailed(const ApiError& error)
{
    QString msg = tr("Failed to get repo download information:\n%1").arg(error.toString());

    seafApplet->warningBox(msg, this);

    setAllInputsEnabled(true);
}

void DownloadRepoDialog::setMergeWithExisting(const QString& localPath) {
    setDirectoryText(localPath);
}
