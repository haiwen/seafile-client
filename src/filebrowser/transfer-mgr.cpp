#include <QDir>
#include <sqlite3.h>
#include <errno.h>
#include <stdio.h>
#include <QDateTime>

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

bool isDownloadForGivenParentDir(const QSharedPointer<FileDownloadTask> task,
                                 const QString& repo_id,
                                 const QString& parent_dir)
{
    return task && task->repoId() == repo_id &&
        ::getParentPath(task->path()) == parent_dir;
}

bool matchDownloadTask(const QSharedPointer<FileDownloadTask>& task,
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

QSharedPointer<FileDownloadTask> TransferManager::addDownloadTask(const Account& account,
                                                                  const QString& repo_id,
                                                                  const QString& path,
                                                                  const QString& local_path)
{
    QSharedPointer<FileDownloadTask> existing_task = getDownloadTask(repo_id, path);
    if (existing_task) {
        return existing_task;
    }

    QSharedPointer<FileDownloadTask> task(new FileDownloadTask(account, repo_id, path, local_path));
    connect(task.data(), SIGNAL(finished(bool)),
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
    if (!pending_downloads_.empty()) {
        QSharedPointer<FileDownloadTask> task = pending_downloads_.dequeue();
        startDownloadTask(task);
    } else {
        current_download_.clear();
    }
}

void TransferManager::startDownloadTask(QSharedPointer<FileDownloadTask> task)
{
    current_download_ = task;
    task->start();
}

QString TransferManager::getDownloadProgress(const QString& repo_id,
                                             const QString& path)
{
    QSharedPointer<FileDownloadTask> task = getDownloadTask(repo_id, path);
    if (!task) {
        return "";
    }
    if (task == current_download_) {
        return task->progress().toString();
    } else {
        return tr("pending");
    }
}

QSharedPointer<FileDownloadTask> TransferManager::getDownloadTask(const QString& repo_id,
                                                                  const QString& path)
{
    if (matchDownloadTask(current_download_, repo_id, path)) {
        return current_download_;
    }
    foreach (const QSharedPointer<FileDownloadTask>& task, pending_downloads_) {
        if (matchDownloadTask(task, repo_id, path)) {
            return task;
        }
    }
    return QSharedPointer<FileDownloadTask>(NULL);
}

bool TransferManager::hasDownloadTask(const QString& repo_id,
                                      const QString& path)
{
    return !getDownloadTask(repo_id, path).isNull();
}

void TransferManager::cancelDownload(const QString& repo_id,
                                     const QString& path)
{
    QSharedPointer<FileDownloadTask> task = getDownloadTask(repo_id, path);
    if (!task) {
        return;
    }
    if (task == current_download_) {
        task->cancel();
    } else {
        pending_downloads_.removeOne(task);
    }
}


QList<QSharedPointer<FileDownloadTask> >
TransferManager::getDownloadTasks(const QString& repo_id,
                                  const QString& parent_dir)
{
    QList<QSharedPointer<FileDownloadTask> > tasks;
    if (isDownloadForGivenParentDir(current_download_, repo_id, parent_dir)) {
        tasks.append(current_download_);
    }
    foreach (const QSharedPointer<FileDownloadTask>& task, pending_downloads_) {
        if (isDownloadForGivenParentDir(task, repo_id, parent_dir)) {
            tasks.append(task);
        }
    }

    return tasks;
}
