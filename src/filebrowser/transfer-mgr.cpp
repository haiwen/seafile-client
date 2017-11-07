#include <errno.h>
#include <stdio.h>
#include <sqlite3.h>

#include <QDateTime>
#include <QTimer>
#include <QDir>

#include "utils/file-utils.h"
#include "utils/utils.h"
#include "configurator.h"
#include "account-mgr.h"
#include "seafile-applet.h"
#include "file-browser-requests.h"
#include "api/requests.h"
#include "tasks.h"
#include "auto-update-mgr.h"
#include "data-cache.h"
#include "data-mgr.h"

#include "transfer-mgr.h"

namespace {

const char *kPendingStatus = "pending";
const char *kStartedStatus = "started";
const char *kErrorStatus = "error";

bool isDownloadForGivenParentDir(const QSharedPointer<FileDownloadTask> &task,
                                 const QString& repo_id,
                                 const QString& parent_dir)
{
    return task && task->repoId() == repo_id &&
        ::getParentPath(task->path()) == parent_dir;
}

bool matchDownloadTask(const QSharedPointer<FileDownloadTask> &task,
                       const QString& repo_id,
                       const QString& path)
{
    return task && task->repoId() == repo_id && task->path() == path;
}

} // namespace

SINGLETON_IMPL(TransferManager)

TransferManager::TransferManager()
{
    account_signature_ = seafApplet->accountManager()->currentAccount().getSignature();
    current_upload_.clear();
    upload_checking_ = false;
}

TransferManager::~TransferManager()
{
}

FileDownloadTask* TransferManager::addDownloadTask(const Account& account,
                                                   const QString& repo_id,
                                                   const QString& path,
                                                   const QString& local_path,
                                                   bool is_save_as_task)
{
    FileDownloadTask* existing_task = getDownloadTask(repo_id, path);
    if (existing_task) {
        return existing_task;
    }

    FileDownloadTask* task = new FileDownloadTask(account, repo_id, path, local_path, is_save_as_task);
    QSharedPointer<FileDownloadTask> shared_task = task->sharedFromThis().objectCast<FileDownloadTask>();
    connect(task, SIGNAL(finished(bool)),
            this, SLOT(onDownloadTaskFinished(bool)));
    if (current_download_) {
        pending_downloads_.enqueue(shared_task);
    } else {
        startDownloadTask(shared_task);
    }
    return task;
}

void TransferManager::onDownloadTaskFinished(bool success)
{
    current_download_.clear();
    if (!pending_downloads_.empty()) {
        const QSharedPointer<FileDownloadTask> &task = pending_downloads_.dequeue();
        startDownloadTask(task);
    }
}

void TransferManager::startDownloadTask(const QSharedPointer<FileDownloadTask> &task)
{
    current_download_ = task;
    task->start();
}

FileDownloadTask* TransferManager::getDownloadTask(const QString& repo_id,
                                                   const QString& path)
{
    if (matchDownloadTask(current_download_, repo_id, path)) {
        return current_download_.data();
    }
    foreach (const QSharedPointer<FileDownloadTask>& task, pending_downloads_) {
        if (matchDownloadTask(task, repo_id, path)) {
            return task.data();
        }
    }
    return NULL;
}

void TransferManager::cancelDownload(const QString& repo_id,
                                     const QString& path)
{
    FileDownloadTask* task = getDownloadTask(repo_id, path);
    if (!task)
        return;
    QSharedPointer<FileDownloadTask> shared_task = task->sharedFromThis().objectCast<FileDownloadTask>();
    if (task == current_download_) {
        task->cancel();
    } else {
        pending_downloads_.removeOne(shared_task);
    }
}

void TransferManager::cancelAllDownloadTasks()
{
    if (current_download_) {
        current_download_->cancel();
        current_download_.clear();
    }
    pending_downloads_.clear();
}

QList<FileDownloadTask*>
TransferManager::getDownloadTasks(const QString& repo_id,
                                  const QString& parent_dir)
{
    QList<FileDownloadTask*> tasks;
    if (isDownloadForGivenParentDir(current_download_, repo_id, parent_dir)) {
        tasks.append(current_download_.data());
    }
    foreach (const QSharedPointer<FileDownloadTask>& task, pending_downloads_) {
        if (isDownloadForGivenParentDir(task, repo_id, parent_dir)) {
            tasks.append(task.data());
        }
    }

    return tasks;
}

bool TransferManager::isValidTask(const FileTaskRecord *task)
{
    return task && task->isValid();
}

void TransferManager::addUploadTask(const QString& repo_id,
                                    const QString& repo_parent_path,
                                    const QString& local_path,
                                    const QString& name,
                                    int folder_task_id)
{
    QString repo_full_path = ::pathJoin(repo_parent_path, name);

    QString task_type = QFileInfo(local_path).isDir() ? "dir" : "file";

    current_upload_task_.account_sign = account_signature_;
    current_upload_task_.type         = task_type;
    current_upload_task_.repo_id      = repo_id;
    current_upload_task_.path         = repo_full_path;
    current_upload_task_.local_path   = local_path;
    current_upload_task_.status       = kPendingStatus;
    current_upload_task_.parent_task  = folder_task_id;

    checkUploadName(&current_upload_task_);
}

void TransferManager::checkUploadName(const FileTaskRecord *params)
{
    if (current_upload_ || upload_checking_) {
        return;
    }

    upload_checking_ = true;
    const Account account = seafApplet->accountManager()->
        getAccountBySignature(params->account_sign);
    if (!account.isValid()) {
        return;
    }

    GetFileDetailRequest* req = new GetFileDetailRequest(
        account, params->repo_id, params->path);
    connect(req, SIGNAL(success(const FileDetailInfo&)),
            this, SLOT(onGetFileDetailSuccess(const FileDetailInfo&)));
    connect(req, SIGNAL(failed(const ApiError&)),
            this, SLOT(onGetFileDetailFailed(const ApiError&)));
    req->send();
}

void TransferManager::onGetFileDetailSuccess(const FileDetailInfo& info)
{
    GetFileDetailRequest *req = qobject_cast<GetFileDetailRequest*>(sender());
    if (req != NULL) {
        QMessageBox::StandardButton ret = seafApplet->yesNoCancelBox(
            tr("File %1 already exists.<br/>"
               "Do you like to overwrite it?<br/>"
               "<small>(Choose No to upload using "
               "an alternative name).</small>").arg(req->path()),
            0, QMessageBox::Cancel);

        if (ret == QMessageBox::Cancel) {
            // cancelUpload(req->repoId(), req->path());
        } else if (ret == QMessageBox::Yes) {
            use_upload_ = false;
        }
        // const int id = getUploadTaskId(req->repoId(), req->path());
        // const FileTaskRecord* task = getFileTaskById(id);
        startUploadTask(&current_upload_task_);
        req->deleteLater();
    }
    upload_checking_ = false;
}

void TransferManager::onGetFileDetailFailed(const ApiError& error)
{
    GetFileDetailRequest *req = qobject_cast<GetFileDetailRequest*>(sender());
    if (req != NULL) {
        use_upload_ = true;
        // const int id = getUploadTaskId(req->repoId(), req->path());
        // const FileTaskRecord* task = getFileTaskById(id);
        startUploadTask(&current_upload_task_);
        req->deleteLater();
    }
    upload_checking_ = false;
}

void TransferManager::startUploadTask(const FileTaskRecord *params)
{
    // if (!isValidTask(params)) {
    //     return;
    // }

    const Account account = seafApplet->accountManager()->
        getAccountBySignature(params->account_sign);
    const QString base_name = ::getBaseName(params->local_path);

    FileUploadTask* task = NULL;
    if (params->parent_task) {
        // bool use_relative_path = false;
        // const FolderTaskRecord *folder_params = getFolderTaskById(params->parent_task);
        // if (!isValidTask(folder_params)) {
        //     return;
        // }
        // updateStatus(folder_params->id, TASK_STARTED, QString(), "FolderTasks");

        // if (QFileInfo(params->local_path).isDir()) {
        //     bool create_parents = true;
        //     if (folder_params->path == params->path) {
        //         // This is the case of an empty top-level folder.
        //         create_parents = false;
        //     }

        //     CreateDirectoryRequest* create_dir_req =
        //         new CreateDirectoryRequest(account, params->repo_id,
        //                                    params->path, create_parents);
        //     connect(create_dir_req, SIGNAL(success()),
        //             this, SLOT(onCreateDirSuccess()));
        //     connect(create_dir_req, SIGNAL(failed(const ApiError&)),
        //             this, SLOT(onCreateDirFailed(const ApiError&)));
        //     updateStatus(params->id, TASK_STARTED);
        //     create_dir_req->send();
        //     return;
        // } else {
        //     use_relative_path = true;
        //     QString parent_path = ::getParentPath(folder_params->path);
        //     if (!parent_path.endsWith('/')) {
        //         parent_path += '/';
        //     }
        //     const int relative_path_diff = params->local_path.size() -
        //                                    folder_params->local_path.size();
        //     const QString relative_path = ::pathJoin(
        //         QFileInfo(folder_params->local_path).fileName(),
        //         ::getParentPath(params->local_path.right(relative_path_diff)));
        //     task = new FileUploadTask(
        //         account,
        //         params->repo_id,
        //         parent_path,
        //         params->local_path,
        //         base_name,
        //         use_upload_,
        //         use_relative_path,
        //         relative_path);
        // }
    } else {
        task = new FileUploadTask(
            account,
            params->repo_id,
            ::getParentPath(params->path),
            params->local_path,
            base_name,
            use_upload_);
    }
    // task->setId(params->id);
    QSharedPointer<FileUploadTask> shared_task =
        task->sharedFromThis().objectCast<FileUploadTask>();
    connect(task, SIGNAL(finished(bool)),
            this, SLOT(onUploadTaskFinished(bool)));

    if (!current_upload_.isNull()) {
        current_upload_.clear();
    }
    current_upload_ = shared_task;
    // updateStatus(current_upload_->id(), TASK_STARTED);
    task->start();
    // emit UploadTaskStarted(current_upload_->id());
}

void TransferManager::onUploadTaskFinished(bool success)
{
    if (success) {
        // const FileTaskRecord* task = getFileTaskById(current_upload_->id());
        // if (!isValidTask(task)) {
        //     return;
        // }
        // emit uploadTaskSuccess(task);
        // deleteUploadTask(current_upload_->id());

        // QFileInfo file_info(current_upload_->localFilePath());
        // if (task->parent_task) {
        //     folder_tasks_[task->parent_task].finished_files_bytes += file_info.size();
        // }
        // upload_progress_->onTaskFinished(file_info.size());
        // refresh_rate_timer_->start(kRefreshRateInterval);
        // emit taskFinished();
    } else {
        // updateStatus(current_upload_->id(), TASK_ERROR,
        //              current_upload_->errorString());
        // if (current_upload_->error() != FileNetworkTask::TaskCanceled) {
        //     emit uploadTaskFailed(getFileTaskById(current_upload_->id()));
        // }

        // emit taskFinished();
    }
    if (!current_upload_.isNull()) {
        current_upload_.clear();
    }

    // startNextUploadTask();
}
