#include <stdio.h>
#include <QtGui>
#include <QTimer>
#include <QPixmap>
#include <QFile>
#include <QFileInfo>
#include <QCoreApplication>

#include "seafile-applet.h"
#include "utils/utils.h"
#include "configurator.h"
#include "settings-mgr.h"
#include "rpc/rpc-client.h"
#include "rpc/local-repo.h"
#include "rpc/clone-task.h"

#include "init-vdrive-dialog.h"

namespace {

const int kCheckDownloadInterval = 2000;

} // namespace


InitVirtualDriveDialog::InitVirtualDriveDialog(const Account& account, QWidget *parent)
    : QDialog(parent),
      account_(account)
{
    setupUi(this);
    mLogo->setPixmap(QPixmap(":/images/seafile-32.png"));
    setWindowTitle(tr("Download Default Library"));
    setWindowIcon(QIcon(":/images/seafile.png"));

    create_default_repo_req_ = NULL;
    download_default_repo_req_ = NULL;

    check_download_timer_ = NULL;
    connect(mYesBtn, SIGNAL(clicked()), this, SLOT(start()));
    connect(mNoBtn, SIGNAL(clicked()), this, SLOT(onCancel()));

    mRunInBackgroundBtn->setVisible(false);
    mFinishBtn->setVisible(false);
    mOpenBtn->setVisible(false);
}

void InitVirtualDriveDialog::start()
{
    // mYesBtn->setEnabled(false);
    // mNoBtn->setEnabled(false);
    mYesBtn->setVisible(false);
    mNoBtn->setVisible(false);
    getDefaultRepo();
}

void InitVirtualDriveDialog::onCancel()
{
    reject();
}

void InitVirtualDriveDialog::getDefaultRepo()
{
    setStatusText(tr("Checking your default library..."));
    get_default_repo_req_ = new GetDefaultRepoRequest(account_);

    connect(get_default_repo_req_, SIGNAL(success(bool, const QString&)),
            this, SLOT(onGetDefaultRepoSuccess(bool, const QString&)));

    connect(get_default_repo_req_, SIGNAL(failed(int)),
            this, SLOT(onGetDefaultRepoFailure(int)));

    get_default_repo_req_->send();
}

void InitVirtualDriveDialog::createDefaultRepo()
{
    setStatusText(tr("Creating the default library..."));
    create_default_repo_req_ = new CreateDefaultRepoRequest(account_);

    connect(create_default_repo_req_, SIGNAL(success(const QString&)),
            this, SLOT(onCreateDefaultRepoSuccess(const QString&)));

    connect(create_default_repo_req_, SIGNAL(failed(int)),
            this, SLOT(onCreateDefaultRepoFailure(int)));

    create_default_repo_req_->send();
}

void InitVirtualDriveDialog::startDownload(const QString& repo_id)
{
    default_repo_id_ = repo_id;

    LocalRepo repo;

    seafApplet->rpcClient()->getLocalRepo(repo_id, &repo);
    if (repo.isValid()) {
        // This repo is already here
        qDebug("The default library has already been downloaded");
        createVirtualDisk(repo);
        success();
        return;
    }

    download_default_repo_req_ = new DownloadRepoRequest(account_, repo_id);

    connect(download_default_repo_req_, SIGNAL(success(const RepoDownloadInfo&)),
            this, SLOT(onDownloadRepoSuccess(const RepoDownloadInfo&)));

    connect(download_default_repo_req_, SIGNAL(failed(int)),
            this, SLOT(onDownloadRepoFailure(int)));

    download_default_repo_req_->send();
}

void InitVirtualDriveDialog::onGetDefaultRepoSuccess(bool exists, const QString& repo_id)
{
    if (!exists) {
        createDefaultRepo();
    } else {
        startDownload(repo_id);
    }
}

void InitVirtualDriveDialog::onGetDefaultRepoFailure(int code)
{
    if (code == 404) {
        fail(tr("Failed to create default library:\n\n"
                "The server version must be 2.1 or higher to support this."));
    } else {
        fail(tr("Failed to get default library: error code %1").arg(code));
    }
}


void InitVirtualDriveDialog::onCreateDefaultRepoSuccess(const QString& repo_id)
{
    startDownload(repo_id);
}

void InitVirtualDriveDialog::onCreateDefaultRepoFailure(int code)
{
    if (code == 404) {
        fail(tr("Failed to create default library:\n\n"
                "The server version must be 2.1 or higher to support this."));
    } else {
        fail(tr("Failed to create default library: error code %1").arg(code));
    }
}

void InitVirtualDriveDialog::onDownloadRepoSuccess(const RepoDownloadInfo& info)
{
    int ret;
    QDir worktree = seafApplet->configurator()->worktreeDir();
    QString default_repo_path = worktree.filePath(info.repo_name);
    QString error;

    if (!worktree.mkpath(info.repo_name)) {
        fail(tr("Failed to create folder \"%1\"").arg(default_repo_path));
        return;
    }

    ret = seafApplet->rpcClient()->cloneRepo(info.repo_id, info.relay_id,
                                             info.repo_name, default_repo_path,
                                             info.token, QString(),
                                             info.magic, info.relay_addr,
                                             info.relay_port, info.email,
                                             info.random_key, info.enc_version,
                                             &error);

    if (ret < 0) {
        fail(tr("Failed to download default library:\n %1").arg(error));
    } else {
        check_download_timer_ = new QTimer(this);
        connect(check_download_timer_, SIGNAL(timeout()), this, SLOT(checkDownloadProgress()));
        check_download_timer_->start(kCheckDownloadInterval);

        setStatusText(tr("Downloading default library..."));

        mRunInBackgroundBtn->setVisible(true);
        connect(mRunInBackgroundBtn, SIGNAL(clicked()), this, SLOT(hide()));
    }
}

void InitVirtualDriveDialog::onDownloadRepoFailure(int code)
{
    fail(tr("Failed to download default library: error code %1").arg(code));
}

void InitVirtualDriveDialog::openVirtualDisk()
{
    accept();
}

void InitVirtualDriveDialog::success()
{
    QString msg = tr("The default library has been downloaded.\n"
                     "You can click the \"Open\" button to open the virtual disk");
    mStatusText->setText(msg);

    mFinishBtn->setVisible(true);
    mOpenBtn->setVisible(true);

    connect(mFinishBtn, SIGNAL(clicked()), this, SLOT(accept()));
    connect(mOpenBtn, SIGNAL(clicked()), this, SLOT(openVirtualDisk()));
}

void InitVirtualDriveDialog::fail(const QString& reason)
{
    ensureVisible();

    setStatusText(reason);
    mFinishBtn->setVisible(true);
    connect(mFinishBtn, SIGNAL(clicked()), this, SLOT(reject()));
}

void InitVirtualDriveDialog::checkDownloadProgress()
{
    // First check for error
    std::vector<CloneTask> tasks;
    if (seafApplet->rpcClient()->getCloneTasks(&tasks) < 0) {
        return;
    }

    CloneTask task;
    for (int i = 0; i < tasks.size(); i++) {
        if (tasks[i].repo_id == default_repo_id_) {
            task = tasks[i];
            break;
        }
    }

    if (!task.isValid()) {
        return;
    }

    if (task.state != "done" && task.state != "error") {
        return;
    }

    check_download_timer_->stop();

    mRunInBackgroundBtn->setVisible(false);
    ensureVisible();

    if (task.state == "error") {
        fail(tr("Error when downloading the default library: %1").arg(task.error_str));
        return;
    }

    // Download is finished. Create the virutal disk in "My Computer".
    LocalRepo repo;
    seafApplet->rpcClient()->getLocalRepo(default_repo_id_, &repo);
    createVirtualDisk(repo);
    success();
}

void InitVirtualDriveDialog::createVirtualDisk(const LocalRepo& repo)
{
    setStatusText(tr("Creating the virtual disk..."));
    Configurator::setVirtualDrive(repo.worktree);
}


void InitVirtualDriveDialog::setStatusText(const QString& status)
{
    mStatusText->setText(status);
}

void InitVirtualDriveDialog::ensureVisible()
{
    show();
    raise();
    activateWindow();
}
