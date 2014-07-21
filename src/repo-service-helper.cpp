#include "repo-service-helper.h"

#include "filebrowser/data-mgr.h"
#include "filebrowser/progress-dialog.h"
#include "filebrowser/file-browser-requests.h"
#include "filebrowser/tasks.h"
#include "filebrowser/auto-update-mgr.h"

#include "ui/set-repo-password-dialog.h"

#include <QDir>
#include "utils/utils.h"
#include "utils/file-utils.h"
#include "seafile-applet.h"

void FileDownloadHelper::openFile(const QString& path, bool work_around_mac_auto_udpate)
{
    if (!::openInNativeExtension(path) && !::showInGraphicalShell(path)) {
        QString file_name = QFileInfo(path).fileName();
        QString msg = QObject::tr("%1 couldn't find an application to open file %2").arg(getBrand()).arg(file_name);
        seafApplet->warningBox(msg);
    }
#ifdef Q_OS_MAC
    MacImageFilesWorkAround::instance()->fileOpened(path);
#endif
}

FileDownloadHelper::FileDownloadHelper(const Account &account, const ServerRepo &repo, const QString &path, QWidget *parent)
  : account_(account), repo_(repo), path_(path), file_name_(QFileInfo(path).fileName()), parent_(parent), req_(NULL) {
}

FileDownloadHelper::~FileDownloadHelper()
{
    onCancel();
    if (req_)
        req_->deleteLater();
}

void FileDownloadHelper::start()
{
    if (req_)
        return;
    const QString file_name = QFileInfo(path_).fileName();
    const QString dirent_path = ::getParentPath(path_);
    req_ = new GetDirentsRequest(account_, repo_.id, dirent_path);
    connect(req_, SIGNAL(success(const QList<SeafDirent> &)),
            this, SLOT(onGetDirentsSuccess(const QList<SeafDirent> &)));
    connect(req_, SIGNAL(failed(const ApiError &)),
            this, SLOT(onGetDirentsFailure(const ApiError &)));
    req_->send();
}

void FileDownloadHelper::onCancel()
{
    if (req_)
        disconnect(req_, 0, this, 0);
}

void FileDownloadHelper::onGetDirentsSuccess(const QList<SeafDirent> &dirents)
{
    bool found_file = false;
    Q_FOREACH(const SeafDirent &dirent, dirents)
    {
        if (dirent.name == file_name_) {
            downloadFile(dirent.id);
            found_file = true;
            break;
        }
    }
    // critally important
    if (!found_file) {
        qWarning("File %s doesn't exist any more", file_name_.toUtf8().data());
    }

}

void FileDownloadHelper::downloadFile(const QString &id)
{
    DataManager data_mgr(account_);

    QString cached_file = data_mgr.getLocalCachedFile(repo_.id, path_, id);
    if (!cached_file.isEmpty()) {
        openFile(cached_file, false);
        return;
    }

    // endless loop for setPasswordDialog
    while(true) {
        FileDownloadTask *task = data_mgr.createDownloadTask(repo_.id, path_);
        FileBrowserProgressDialog dialog(task, parent_);
        if (dialog.exec()) {
            QString full_path =
                data_mgr.getLocalCachedFile(repo_.id, path_, task->fileId());
            if (!full_path.isEmpty())
                openFile(full_path, true);
            break;
        }
        // if the user canceled the task, don't bother it
        if (task->error() == FileNetworkTask::TaskCanceled)
            break;
        // if the repo_sitory is encrypted and password is incorrect
        if (repo_.encrypted && task->httpErrorCode() == 400) {
            SetRepoPasswordDialog password_dialog(repo_, parent_);
            if (password_dialog.exec())
                continue;
            // the user canceled the dialog? skip
            break;
        }
        QString msg =
            QObject::tr("Unable to download item \"%1\"").arg(path_);
        seafApplet->warningBox(msg);
        break;
    }
}

