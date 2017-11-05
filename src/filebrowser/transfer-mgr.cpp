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

const int kRefreshRateInterval = 1000;

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

// class TransferProgress
//        |----- ... ---|-----|-----|-----|----- ... --------|
//    task_start         cycle cycle cycle               task_finish
//         |-----------------------------------|-------------|
//    cycle_begin                          task_finish  cycle_end
//
//    TODO:
//         |----------------------------------|------------|-------------|
//    cycle_begin                         task_start   task_finish  cycle_end
//         |----------------|-----------------|------------|-------------|
//    cycle_begin   previous_task_start   task_start   task_finish  cycle_end
class TransferProgress {
public:
    TransferProgress();

    void reset();
    void updateRate(quint64 transferred_bytes);
    uint currentRate() const { return current_rate_;}
    void onTaskFinished(quint64 finished_task_size, int remaining_msec);
    void onCycleFinished();

private:
    quint64 current_task_previous_transferred_;
    uint current_rate_;
    bool current_cycle_had_tasks_;
};

TransferProgress::TransferProgress()
{
    reset();
}

void TransferProgress::reset() {
    current_rate_ = 0;
    current_task_previous_transferred_ = 0;
    current_cycle_had_tasks_ = false;
}

void TransferProgress::updateRate(quint64 current_task_transferred_bytes)
{
    current_rate_ = current_task_transferred_bytes - current_task_previous_transferred_;
    current_task_previous_transferred_ = current_task_transferred_bytes;
    current_cycle_had_tasks_ = true;
}

void TransferProgress::onTaskFinished(quint64 finished_task_size,  int remaining_msec)
{
    current_rate_ = (finished_task_size - current_task_previous_transferred_)
                    * 1000 / (1000 - remaining_msec) ;
    current_task_previous_transferred_ = 0;
    current_cycle_had_tasks_ = true;
}

void TransferProgress::onCycleFinished()
{
    if (current_cycle_had_tasks_) {
        current_cycle_had_tasks_ = false;
    } else {
        reset();
    }
}




SINGLETON_IMPL(TransferManager)

TransferManager::TransferManager()
{
    download_progress_ = new TransferProgress;
    upload_progress_ = new TransferProgress;

    refresh_rate_timer_ = new QTimer(this);
    connect(refresh_rate_timer_, SIGNAL(timeout()),
            this, SLOT(refreshRate()));
    refresh_rate_timer_->start(kRefreshRateInterval);
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
    if (success) {
        quint64 finished_task_size = current_download_->progress().total;
        download_progress_->onTaskFinished(finished_task_size, refresh_rate_timer_->remainingTime());
    }

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

void TransferManager::refreshRate()
{
    if (current_download_) {
        const quint64 current_task_downloaded_bytes =
            current_download_->progress().transferred;
        download_progress_->updateRate(current_task_downloaded_bytes);
    } else {
        download_progress_->onCycleFinished();
    }
}

void TransferManager::getTransferRate(uint *upload_rate,
                                      uint *download_rate)
{
    *upload_rate = upload_progress_->currentRate();
    *download_rate = download_progress_->currentRate();
}
