#include <QtGui>
#include <jansson.h>

#include "account-mgr.h"
#include "utils/utils.h"
#include "seafile-applet.h"
#include "rpc/rpc-client.h"
#include "configurator.h"
#include "api/requests.h"
#include "api/api-error.h"
#include "api/server-repo.h"
#include "download-repo-dialog.h"

namespace {

} // namespace

DownloadRepoDialog::DownloadRepoDialog(const Account& account,
                                       const ServerRepo& repo,
                                       QWidget *parent)
    : QDialog(parent),
      repo_(repo),
      account_(account),
      mode_(CREATE_NEW_FOLDER)
{
    setupUi(this);
    setWindowTitle(tr("Sync library \"%1\"").arg(repo_.name));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    mRepoIcon->setPixmap(repo.getPixmap());
    mRepoName->setText(repo_.name);

    mDirectory->setPlaceholderText(tr("Choose a folder"));

    if (repo_.encrypted) {
        mPassword->setVisible(true);
        mPasswordLabel->setVisible(true);
    } else {
        mPassword->setVisible(false);
        mPasswordLabel->setVisible(false);
    }

    int height = 250;
#ifdef Q_WS_MAC
    layout()->setContentsMargins(8, 9, 9, 5);
    layout()->setSpacing(6);
    verticalLayout_3->setSpacing(6);
#endif
    if (repo.encrypted) {
        height += 100;
    }
    setMinimumHeight(height);
    setMaximumHeight(height);

    saved_create_new_path_ = seafApplet->configurator()->worktreeDir();

    updateSyncMode();

    connect(mSwitchModeHint, SIGNAL(linkActivated(const QString&)),
            this, SLOT(switchMode()));

    connect(mChooseDirBtn, SIGNAL(clicked()), this, SLOT(chooseDirAction()));
    connect(mOkBtn, SIGNAL(clicked()), this, SLOT(onOkBtnClicked()));
}

void DownloadRepoDialog::switchMode()
{
    if (mode_ == CREATE_NEW_FOLDER) {
        mode_ = MERGE_WITH_EXISTING_FOLDER;
    } else {
        mode_ = CREATE_NEW_FOLDER;
    }

    updateSyncMode();
}

void DownloadRepoDialog::updateSyncMode()
{
    QString switch_hint_text;
    QString op_text;
    const QString link_template = "<a style=\"color:#FF9A2A\" href=\"#\">%1</a>";

    mDirectory->clear();

    QString OR = tr("or");
    if (mode_ == CREATE_NEW_FOLDER) {
        QString link = link_template.arg(tr("sync with an existing folder"));
        switch_hint_text = QString("%1 %2").arg(OR).arg(link);

        op_text = tr("Create a new sync folder at:");

        if (!saved_create_new_path_.isNull()) {
            setDirectoryText(saved_create_new_path_);
        }

    } else {
        QString link = link_template.arg(tr("create a new sync folder"));
        switch_hint_text = QString("%1 %2").arg(OR).arg(link);

        op_text = tr("Sync with this existing folder:");

        if (!saved_merge_existing_path_.isNull()) {
            setDirectoryText(saved_merge_existing_path_);
        }
    }

    mOperationText->setText(op_text);
    mSwitchModeHint->setText(switch_hint_text);
}

void DownloadRepoDialog::setDirectoryText(const QString& path)
{
    QString text = path;
    if (text.endsWith("/")) {
        text.resize(text.size() - 1);
    }

    mDirectory->setText(text);

    if (mode_ == CREATE_NEW_FOLDER) {
        saved_create_new_path_ = text;
    } else {
        saved_merge_existing_path_ = text;
    }
}

void DownloadRepoDialog::chooseDirAction()
{
    const QString &wt = seafApplet->configurator()->worktreeDir();
    QString dir = QFileDialog::getExistingDirectory(this, tr("Please choose a directory"),
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

    DownloadRepoRequest *req = new DownloadRepoRequest(account_, repo_.id);
    connect(req, SIGNAL(success(const RepoDownloadInfo&)),
            this, SLOT(onDownloadRepoRequestSuccess(const RepoDownloadInfo&)));
    connect(req, SIGNAL(failed(const ApiError&)),
            this, SLOT(onDownloadRepoRequestFailed(const ApiError&)));
    req->send();
}

bool DownloadRepoDialog::validateInputs()
{
    QString local_dir, password;
    setDirectoryText(mDirectory->text().trimmed());
    if (mDirectory->text().isEmpty()) {
        QMessageBox::warning(this, getBrand(),
                             tr("Please choose the folder to sync"),
                             QMessageBox::Ok);
        return false;
    }
    QDir dir(mDirectory->text());
    if (!dir.exists()) {
        QMessageBox::warning(this, getBrand(),
                             tr("The folder does not exist"),
                             QMessageBox::Ok);
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
    local_dir = mDirectory->text();
    return true;
}

void DownloadRepoDialog::setAllInputsEnabled(bool enabled)
{
    mDirectory->setEnabled(enabled);
    mChooseDirBtn->setEnabled(enabled);
    mPassword->setEnabled(enabled);
    mOkBtn->setEnabled(enabled);
}

namespace {

QString buildMoreInfo(ServerRepo& repo)
{
    json_t *object = NULL;
    const char *info = NULL;

    object = json_object();
    json_object_set_new(object, "is_readonly", json_integer(repo.readonly));
    
    info = json_dumps(object, 0);
    QString ret = QString::fromUtf8(info);
    json_decref (object);
    return ret;
}

}

void DownloadRepoDialog::onDownloadRepoRequestSuccess(const RepoDownloadInfo& info)
{
    QString worktree = mDirectory->text();
    QString password = repo_.encrypted ? mPassword->text() : QString();
    int ret;
    QString error;
    QString more_info = buildMoreInfo(repo_);

    if (mode_ == MERGE_WITH_EXISTING_FOLDER) {
        ret = seafApplet->rpcClient()->cloneRepo(info.repo_id, info.repo_version,
                                                 info.relay_id,
                                                 repo_.name, worktree,
                                                 info.token, password,
                                                 info.magic, info.relay_addr,
                                                 info.relay_port, info.email,
                                                 info.random_key, info.enc_version,
                                                 more_info,
                                                 &error);
    } else {
        ret = seafApplet->rpcClient()->downloadRepo(info.repo_id, info.repo_version,
                                                    info.relay_id,
                                                    repo_.name, worktree,
                                                    info.token, password,
                                                    info.magic, info.relay_addr,
                                                    info.relay_port, info.email,
                                                    info.random_key, info.enc_version,
                                                    more_info,
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
