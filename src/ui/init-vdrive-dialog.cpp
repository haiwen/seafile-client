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
#include "api/requests.h"
#include "api/api-error.h"
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
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    setStatusIcon(":/images/download-48.png");

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

    connect(get_default_repo_req_, SIGNAL(failed(const ApiError&)),
            this, SLOT(onGetDefaultRepoFailure(const ApiError&)));

    get_default_repo_req_->send();
}

void InitVirtualDriveDialog::createDefaultRepo()
{
    setStatusText(tr("Creating the default library..."));
    create_default_repo_req_ = new CreateDefaultRepoRequest(account_);

    connect(create_default_repo_req_, SIGNAL(success(const QString&)),
            this, SLOT(onCreateDefaultRepoSuccess(const QString&)));

    connect(create_default_repo_req_, SIGNAL(failed(const ApiError&)),
            this, SLOT(onCreateDefaultRepoFailure(const ApiError&)));

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
        default_repo_path_ = repo.worktree;
        finish();
        return;
    }

    download_default_repo_req_ = new DownloadRepoRequest(account_, repo_id);

    connect(download_default_repo_req_, SIGNAL(success(const RepoDownloadInfo&)),
            this, SLOT(onDownloadRepoSuccess(const RepoDownloadInfo&)));

    connect(download_default_repo_req_, SIGNAL(failed(const ApiError&)),
            this, SLOT(onDownloadRepoFailure(const ApiError&)));

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

void InitVirtualDriveDialog::onGetDefaultRepoFailure(const ApiError& error)
{
    if (error.type() == ApiError::HTTP_ERROR && error.httpErrorCode() == 404) {
        fail(tr("Failed to create default library:\n\n"
                "The server version must be 2.1 or higher to support this."));
    } else {
        fail(tr("Failed to get default library:\n%1").arg(error.toString()));
    }
}


void InitVirtualDriveDialog::onCreateDefaultRepoSuccess(const QString& repo_id)
{
    startDownload(repo_id);
}

void InitVirtualDriveDialog::onCreateDefaultRepoFailure(const ApiError& error)
{
    if (error.type() == ApiError::HTTP_ERROR && error.httpErrorCode() == 404) {
        fail(tr("Failed to create default library:\n\n"
                "The server version must be 2.1 or higher to support this."));
    } else {
        fail(tr("Failed to create default library:\n%1").arg(error.toString()));
    }
}

void InitVirtualDriveDialog::onDownloadRepoSuccess(const RepoDownloadInfo& info)
{
    int ret;
    QString worktree = seafApplet->configurator()->worktreeDir();
    QString error;

    ret = seafApplet->rpcClient()->downloadRepo(info.repo_id,
                                                info.repo_version, info.relay_id,
                                                info.repo_name, worktree,
                                                info.token, QString(),
                                                info.magic, info.relay_addr,
                                                info.relay_port, info.email,
                                                info.random_key, info.enc_version,
                                                QString(),
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

void InitVirtualDriveDialog::onDownloadRepoFailure(const ApiError& error)
{
    fail(tr("Failed to download default library:\n%1").arg(error.toString()));
}

void InitVirtualDriveDialog::openVirtualDisk()
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(default_repo_path_));
    accept();
}

void InitVirtualDriveDialog::finish()
{
    QString msg = tr("The default library has been downloaded.\n"
                     "You can click the \"Open\" button to view it.");
    setStatusText(msg);
    setStatusIcon(":/images/ok-48.png");

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
    default_repo_path_ = repo.worktree;
    finish();
}

void InitVirtualDriveDialog::createVirtualDisk(const LocalRepo& repo)
{
    setStatusText(tr("Creating the virtual disk..."));
    Configurator::setVirtualDrive(repo.worktree, repo.name);
}


void InitVirtualDriveDialog::setStatusText(const QString& status)
{
    mStatusText->setText(status);
}

void InitVirtualDriveDialog::setStatusIcon(const QString& path)
{
    mStatusIcon->setPixmap(QPixmap(path));
}

void InitVirtualDriveDialog::ensureVisible()
{
    show();
    raise();
    activateWindow();
}
