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
#include "tasks.h"
#include "auto-update-mgr.h"
#include "data-cache.h"
#include "data-mgr.h"

#include "transfer-mgr.h"

namespace {

bool isDownloadForGivenParentDir(FileDownloadTask* task,
                                 const QString& repo_id,
                                 const QString& parent_dir)
{
    return task && task->repoId() == repo_id &&
        ::getParentPath(task->path()) == parent_dir;
}

bool matchDownloadTask(FileDownloadTask* task,
                       const QString& repo_id,
                       const QString& path)
{
    return task && task->repoId() == repo_id && task->path() == path;
}

} // namespace

SINGLETON_IMPL(TransferManager)

TransferManager::TransferManager()
{
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
    connect(task, SIGNAL(finished(bool)),
            this, SLOT(onDownloadTaskFinished(bool)));
    if (current_download_) {
        pending_downloads_.enqueue(task);
    } else {
        startDownloadTask(task);
    }
    return task;
}

void TransferManager::onDownloadTaskFinished(bool success)
{
    FileDownloadTask *task = (FileDownloadTask *)sender();
    if (task == current_download_) {
        current_download_ = nullptr;
        if (!pending_downloads_.empty()) {
            FileDownloadTask* task = pending_downloads_.dequeue();
            startDownloadTask(task);
        }
    } else {
        pending_downloads_.removeOne(task);
    }
}

void TransferManager::startDownloadTask(FileDownloadTask* task)
{
    current_download_ = task;
    task->start();
}

FileDownloadTask* TransferManager::getDownloadTask(const QString& repo_id,
                                                   const QString& path)
{
    if (matchDownloadTask(current_download_, repo_id, path)) {
        return current_download_;
    }
    foreach (FileDownloadTask* task, pending_downloads_) {
        if (matchDownloadTask(task, repo_id, path)) {
            return task;
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
    // If the task is a pending one, it would be removed from queue in
    // ::onDownloadTaskFinished slot.
    task->cancel();
}

void TransferManager::cancelAllDownloadTasks()
{
    if (current_download_) {
        current_download_->cancel();
        current_download_= nullptr;
    }
    pending_downloads_.clear();
}

QList<FileDownloadTask*>
TransferManager::getDownloadTasks(const QString& repo_id,
                                  const QString& parent_dir)
{
    QList<FileDownloadTask*> tasks;
    if (isDownloadForGivenParentDir(current_download_, repo_id, parent_dir)) {
        tasks.append(current_download_);
    }
    foreach (FileDownloadTask* task, pending_downloads_) {
        if (isDownloadForGivenParentDir(task, repo_id, parent_dir)) {
            tasks.append(task);
        }
    }

    return tasks;
}
