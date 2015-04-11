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
    QSharedPointer<FileDownloadTask> shared_task = task->sharedFromThis().objectCast<FileDownloadTask>();
    if (!task) {
        return;
    }
    if (task == current_download_) {
        task->cancel();
    } else {
        pending_downloads_.removeOne(shared_task);
    }
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
