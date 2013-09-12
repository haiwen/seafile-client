#include <QtGui>

#include "account-mgr.h"
#include "utils/utils.h"
#include "seafile-applet.h"
#include "rpc/rpc-client.h"
#include "configurator.h"
#include "api/requests.h"
#include "api/server-repo.h"
#include "download-repo-dialog.h"

DownloadRepoDialog::DownloadRepoDialog(const Account& account,
                                       const ServerRepo& repo,
                                       QWidget *parent)
    : QDialog(parent),
      account_(account),
      repo_(repo),
      sync_with_existing_(false)
{
    setupUi(this);
    setWindowTitle(tr("Download library \"%1\"").arg(repo_.name));

    // mRepoIcon->setPixmap(repo_.getPixmap().scaled(32, 32));
    // mRepoName->setText(QString().sprintf("<b>%s</b>", toCStr(repo_.name)));
    mDirectory->setText(seafApplet->configurator()->worktreeDir());

    mSyncHint->setVisible(false);

    if (repo_.encrypted) {
        mPassword->setVisible(true);
        mPasswordLabel->setVisible(true);
    } else {
        mPassword->setVisible(false);
        mPasswordLabel->setVisible(false);
    }

    connect(mChooseDirBtn, SIGNAL(clicked()), this, SLOT(chooseDirAction()));
    connect(mSwitchToSyncBtn, SIGNAL(clicked()), this, SLOT(switchToSync()));
    connect(mOkBtn, SIGNAL(clicked()), this, SLOT(onOkBtnClicked()));
}

void DownloadRepoDialog::chooseDirAction()
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
}

void DownloadRepoDialog::onOkBtnClicked()
{
    if (!validateInputs()) {
        return;
    }

    setAllInputsEnabled(false);

    DownloadRepoRequest *req = new DownloadRepoRequest(account_, repo_);
    connect(req, SIGNAL(success(const RepoDownloadInfo&)),
            this, SLOT(onDownloadRepoRequestSuccess(const RepoDownloadInfo&)));
    connect(req, SIGNAL(failed(int)),
            this, SLOT(onDownloadRepoRequestFailed(int)));
    req->send();
}

bool DownloadRepoDialog::validateInputs()
{
    QString local_dir, password;
    mDirectory->setText(mDirectory->text().trimmed());
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
    if (repo_.encrypted) {
        mPasswordLabel->setText(mPassword->text().trimmed());
        if (mPassword->text().isEmpty()) {
            QMessageBox::warning(this, tr("Seafile"),
                                 tr("Please enter the password"),
                                 QMessageBox::Ok);
            return false;
        }
    }

    password = mPassword->text();
    local_dir = mDirectory->text();
    return true;
}

void DownloadRepoDialog::switchToSync()
{
    mSwitchToSyncFrame->setVisible(false);
    mSyncHint->setVisible(true);

    mDirectory->clear();

    setWindowTitle(tr("Sync library \"%1\"").arg(repo_.name));
    mDownloadLabel->setText(tr("Sync this library with:"));

    sync_with_existing_ = true;
}

void DownloadRepoDialog::setAllInputsEnabled(bool enabled)
{
    mDirectory->setEnabled(enabled);
    mChooseDirBtn->setEnabled(enabled);
    mPassword->setEnabled(enabled);
    mSwitchToSyncBtn->setEnabled(enabled);
    mOkBtn->setEnabled(enabled);
}

void DownloadRepoDialog::onDownloadRepoRequestSuccess(const RepoDownloadInfo& info)
{
    QString worktree = mDirectory->text();
    QString password = repo_.encrypted ? mPassword->text() : QString();
    int ret;
    QString error;
    if (sync_with_existing_) {
        ret = seafApplet->rpcClient()->cloneRepo(repo_.id, info.relay_id,
                                                 repo_.name, worktree,
                                                 info.token, password,
                                                 info.magic, info.relay_addr,
                                                 info.relay_port, info.email,
                                                 &error);
    } else {
        ret = seafApplet->rpcClient()->downloadRepo(repo_.id, info.relay_id,
                                                    repo_.name, worktree,
                                                    info.token, password,
                                                    info.magic, info.relay_addr,
                                                    info.relay_port, info.email,
                                                    &error);
    }

    if (ret < 0) {
        QMessageBox::warning(this, tr("Seafile"),
                             tr("Failed to add download task:\n %1").arg(error),
                             QMessageBox::Ok);
        setAllInputsEnabled(true);
    } else {
        done(QDialog::Accepted);
    }
}


void DownloadRepoDialog::onDownloadRepoRequestFailed(int code)
{
    QMessageBox::warning(this, tr("Seafile"),
                         tr("Failed to get repo download information"),
                         QMessageBox::Ok);

    setAllInputsEnabled(true);
}
