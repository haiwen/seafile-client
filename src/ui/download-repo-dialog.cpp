#include <QtGlobal>

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
#include <QtWidgets>
#else
#include <QtGui>
#endif
#include <QDirIterator>
#include <QTimer>

#include <jansson.h>

#include "account-mgr.h"
#include "settings-mgr.h"
#include "utils/utils.h"
#include "utils/file-utils.h"
#include "seafile-applet.h"
#include "rpc/rpc-client.h"
#include "configurator.h"
#include "api/requests.h"
#include "api/api-error.h"
#include "api/server-repo.h"
#include "repo-service.h"
#include "download-repo-dialog.h"

namespace {
const int kAlternativeTryTimes = 20;
bool inline isPathInWorktree(const QString& worktree, const QString &path)
{
    if (path == worktree) {
        return true;
    }
    QDir dir(worktree);
    if (dir.relativeFilePath(QFileInfo(path).absoluteFilePath()).startsWith("."))
        return false;
    return true;
}

bool isPathConflictWithExistingRepo(const QString &path, QString *repo_name) {
    RepoService::instance()->refreshLocalRepoList();
    const std::vector<LocalRepo> & repos = RepoService::instance()->localRepos();
    for (unsigned i = 0; i < repos.size(); ++i) {
        // compare case insensitive file names as well
        if (QFileInfo(repos[i].worktree) == QFileInfo(path)) {
            *repo_name = repos[i].name;
            return true;
        }
    }
    return false;
}

QString getAlternativePath(const QString &dir_path, const QString &name) {
    QDir dir = QDir(dir_path);
    QFileInfo file;
    file = QFileInfo(dir.filePath(name));
    int i;
    for (i = 1; i < kAlternativeTryTimes; ++i) {
        if (!file.exists() && dir.mkdir(file.fileName()))
            return file.absoluteFilePath();
        file = QFileInfo(dir.filePath(name + "-" + QString::number(i)));
    }

    return QString();
}

inline QString getOperatingText(const ServerRepo &repo) {
    if (!repo.isSubfolder()) {
        return QObject::tr("Sync this library to:");
    } else {
        return QObject::tr("Sync this folder to:");
    }
}
} // anonymous namespace

DownloadRepoDialog::DownloadRepoDialog(const Account& account,
                                       const ServerRepo& repo,
                                       const QString& password,
                                       QWidget *parent)
    : QDialog(parent),
      repo_(repo),
      account_(account),
      merge_without_question_(false),
      resync_mode_(false)
{
    manual_merge_mode_ = false;
    setupUi(this);
    if (!repo.isSubfolder()) {
        setWindowTitle(tr("Sync library \"%1\"").arg(repo_.name));
    }
    else {
        setWindowTitle(tr("Sync folder \"%1\"").arg(repo.parent_path));
    }
    mDirectory->setPlaceholderText(getOperatingText(repo_));
    mDirectory->setReadOnly(true);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowIcon(QIcon(":/images/seafile.png"));

    mRepoIcon->setPixmap(repo.getPixmap());
    mRepoName->setText(repo_.name);
    mOperationText->setText(tr("Sync to folder:"));

    if (repo_.encrypted) {
        if (!password.isEmpty()) {
            mPassword->setText(password);
            mPassword->setReadOnly(true);
        }
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
        height += 50;
    }
    setMinimumHeight(height);
    setMaximumHeight(height);

    setDirectoryText(seafApplet->configurator()->worktreeDir());

    connect(mSwitchModeHint, SIGNAL(linkActivated(const QString &)),
            this, SLOT(switchMode()));

    updateSyncMode();

    mMergeHint->hide();

    connect(mChooseDirBtn, SIGNAL(clicked()), this, SLOT(chooseDirAction()));
    connect(mOkBtn, SIGNAL(clicked()), this, SLOT(onOkBtnClicked()));

    QTimer::singleShot(1000, this, SLOT(startResync()));
}

void DownloadRepoDialog::startResync()
{
    if (!resync_mode_) {
        return;
    }
    onOkBtnClicked();
}

void DownloadRepoDialog::switchMode()
{
    manual_merge_mode_ = !manual_merge_mode_;

    QString path = mDirectory->text().trimmed();
    if (!path.isEmpty()) {
        // from download new to merge existing
        if (manual_merge_mode_) {
            setDirectoryText(::pathJoin(path, repo_.name));
        } else {
            // from merge existing to download new
            if (::getBaseName(path) == repo_.name) {
                setDirectoryText(::getParentPath(path));
            }
        }
    }

    updateSyncMode();
}

void DownloadRepoDialog::updateSyncMode()
{
    if (account_.hasDisableSyncWithAnyFolder()) {
        sync_with_existing_ = false;
        manual_merge_mode_ = false;
        mOperationText->setVisible(false);
        mSwitchModeHint->setVisible(false);
        return;
    }
    QString switch_hint_text;
    QString op_text;
    const QString link_template = "<a style=\"color:#FF9A2A\" href=\"#\">%1</a>";

    QString OR = tr("or");
    if (!manual_merge_mode_) {
        op_text = getOperatingText(repo_);
        sync_with_existing_ = false;

        QString link = link_template.arg(tr("sync with an existing folder"));
        switch_hint_text = QString("%1 %2").arg(OR).arg(link);

    } else {
        QString link = link_template.arg(tr("create a new sync folder"));
        switch_hint_text = QString("%1 %2").arg(OR).arg(link);
        sync_with_existing_ = true;

        op_text = tr("Sync with this existing folder:");

        if (!alternative_path_.isNull()) {
            setDirectoryText(alternative_path_);
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

    if (manual_merge_mode_) {
        alternative_path_ = text;
    }
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

bool DownloadRepoDialog::validateInputDirectory()
{
    QDir dir(mDirectory->text());
    if (!dir.exists()) {
        seafApplet->warningBox(tr("The folder does not exist"));
        return false;
    }
    return true;
}

bool DownloadRepoDialog::validateInputs()
{
    setDirectoryText(mDirectory->text().trimmed());
    if (mDirectory->text().isEmpty()) {
        seafApplet->warningBox(tr("Please choose the folder to sync."));
        return false;
    }
    if (account_.hasDisableSyncWithAnyFolder() &&
        !isPathInWorktree(seafApplet->configurator()->worktreeDir(), mDirectory->text())) {
        seafApplet->warningBox(
            tr("Your organization disables putting a library outside %1 folder.").arg(getBrand()));
        return false;
    }
    if (repo_.encrypted) {
        mPassword->setText(mPassword->text().trimmed());
        if (mPassword->text().isEmpty()) {
            seafApplet->warningBox(tr("Please enter the password"));
            return false;
        }
    }

    if (!validateInputDirectory()) {
        return false;
    }

    if (manual_merge_mode_) {
        return true;
    }

    sync_with_existing_ = false;
    alternative_path_ = "";

    QString path = QDir(mDirectory->text()).absoluteFilePath(repo_.name);
    QFileInfo fileinfo = QFileInfo(path);
    if (fileinfo.exists()) {
        sync_with_existing_ = true;
        // exist and but not a directory ?
        if (!fileinfo.isDir()) {
            seafApplet->warningBox(
                tr("Conflicting with existing file \"%1\", please choose a different folder.").arg(path));
            return false;
        }
        // exist and but conflicting?
        QString repo_name;
        if (isPathConflictWithExistingRepo(path, &repo_name)) {
            seafApplet->warningBox(
                tr("Conflicting with existing library \"%1\", please choose a different folder.").arg(repo_name));
            return false;
        }
        QMessageBox::StandardButton ret = merge_without_question_
            ? QMessageBox::Yes
            : seafApplet->yesNoCancelBox(
                tr("The folder \"%1\" already exists. Are you sure to sync with it (contents will be merged)?")
                .arg(path) + QString("<br/><br/><small>%1</small>").arg(tr("Click No to sync with a new folder instead")),
                this, QMessageBox::Yes);
        if (ret == QMessageBox::Cancel)
            return false;
        if (ret == QMessageBox::No) {
            QString new_path = getAlternativePath(mDirectory->text(), repo_.name);
            if (new_path.isEmpty()) {
                seafApplet->warningBox(tr("Unable to find an alternative folder name").arg(path));
                return false;
            }
            alternative_path_ = new_path;
        }
    }

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
        if (alternative_path_.isEmpty())
            worktree = QDir(worktree).absoluteFilePath(repo_.name);
        else
            worktree = alternative_path_;
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
        if (error == "Worktree conflicts system path") {
            error = QObject::tr("The path \"%1\" conflicts with system path").arg(worktree);
        } else if (error == "Worktree conflicts existing repo") {
            error = QObject::tr("The path \"%1\" conflicts with an existing library").arg(worktree);
        } else if (error == "Library name contains invalid characters such as ':', '*', '|', '?'") {
            error = QObject::tr("Library name contains invalid characters such as ':', '*', '|', '?'");
        }
        seafApplet->warningBox(tr("Failed to add download task:\n %1").arg(error), this);
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
    merge_without_question_ = true;
    setDirectoryText(localPath);
}

void DownloadRepoDialog::setResyncMode() {
    resync_mode_ = true;
    setAllInputsEnabled(false);
}
