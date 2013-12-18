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
    setWindowTitle(tr("Seafile Initialization"));
    setWindowIcon(QIcon(":/images/seafile.png"));
    // createLoadingView();

    create_default_repo_req_ = NULL;
    download_default_repo_req_ = NULL;

    check_download_timer_ = NULL;
    connect(mOkBtn, SIGNAL(clicked()), this, SLOT(start()));
    connect(mCancelBtn, SIGNAL(clicked()), this, SLOT(onCancel()));
}

void InitVirtualDriveDialog::start()
{
    mOkBtn->setEnabled(false);
    mCancelBtn->setEnabled(false);
    getDefaultRepo();
}

void InitVirtualDriveDialog::onCancel()
{
    seafApplet->settingsManager()->setDefaultLibraryAlreadySetup();
    reject();
}

void InitVirtualDriveDialog::createLoadingView()
{
    // QVBoxLayout *layout = new QVBoxLayout;
    // mLoadingView->setLayout(layout);

    // QMovie *gif = new QMovie(":/images/loading.gif");
    // QLabel *label = new QLabel;
    // label->setMovie(gif);
    // label->setAlignment(Qt::AlignCenter);
    // gif->start();

    // layout->addWidget(label);
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
    setStatusText(tr("Create a default library..."));
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
        setVDrive(repo);
        onSuccess();
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

        setStatusText(tr("downloading default library..."));
    }
}

void InitVirtualDriveDialog::onDownloadRepoFailure(int code)
{
    fail(tr("Failed to download default library: error code %1").arg(code));
}

void InitVirtualDriveDialog::onSuccess()
{
    QString msg = tr("The default library has been setup. Please click the \"Finish\" button");
    mStatusText->setText(msg);

    mCancelBtn->setVisible(false);
    mOkBtn->setEnabled(true);
    mOkBtn->disconnect();
    mOkBtn->setText("Finish");
    connect(mOkBtn, SIGNAL(clicked()), this, SLOT(accept()));
}

void InitVirtualDriveDialog::fail(const QString& reason)
{
    seafApplet->warningBox(reason);
    reject();
}

void InitVirtualDriveDialog::checkDownloadProgress()
{
    LocalRepo repo;
    seafApplet->rpcClient()->getLocalRepo(default_repo_id_, &repo);
    if (!repo.isValid()) {
        return;
    }

    check_download_timer_->stop();
    setStatusText(tr("downloading default library... done"));

    setVDrive(repo);
    onSuccess();
}

void InitVirtualDriveDialog::setVDrive(const LocalRepo& repo)
{
    setStatusText(tr("updating default libray..."));
    Configurator::setVirtualDrive(repo.worktree);
    seafApplet->settingsManager()->setDefaultLibraryAlreadySetup();
}


void InitVirtualDriveDialog::setStatusText(const QString& status)
{
    mStatusText->setText(status);
}
