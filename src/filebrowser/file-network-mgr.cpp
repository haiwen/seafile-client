#include "file-network-mgr.h"

#include <QThread>
#include <QDebug>

#include "account.h"
#include "utils/utils.h"
#include "api/api-error.h"
#include "network/task.h"
#include "file-browser-requests.h"

FileNetworkTask::FileNetworkTask(const SeafileNetworkTaskType type,
    FileNetworkManager *network_mgr,
    const QString &file_name,
    const QString &parent_dir,
    const QString &file_location,
    const QString &file_oid)
    :   file_name_(file_name), parent_dir_(parent_dir),
        file_path_(parent_dir_ + file_name_),
        file_location_(file_location), file_oid_(file_oid),
        processed_bytes_(0), total_bytes_(0),
        status_(SEAFILE_NETWORK_TASK_STATUS_FRESH),
        type_(type), network_task_(NULL), network_mgr_(network_mgr)
{
    connect(this, SIGNAL(start()), this, SIGNAL(started()));

    //connection with network_mgr
    connect(this, SIGNAL(started()), network_mgr_ ,SLOT(onTaskStarted()));
    connect(this, SIGNAL(aborted()), network_mgr_ ,SLOT(onTaskFinished()));
    connect(this, SIGNAL(finished()), network_mgr_ ,SLOT(onTaskFinished()));

    if (type_ == SEAFILE_NETWORK_TASK_UPLOAD) {

        // setup prefetch task
        GetFileUploadRequest *prefetch_task = new GetFileUploadRequest(
          network_mgr_->account_, network_mgr_->repo_id_, file_path_);
        connect(prefetch_task, SIGNAL(success(const QString&)),
                this, SLOT(onPrefetchFinished(const QString&)));
        connect(prefetch_task, SIGNAL(failed(const ApiError&)),
                this, SLOT(onAborted()));
        prefetch_task->setParent(this);
        prefetch_task->send();

    } else if (type_ == SEAFILE_NETWORK_TASK_DOWNLOAD) {

        // find if cached in the list
        if (!file_oid_.isEmpty()) {
            // if hit the cache while not modify, since oid might be inaccurate
            if (network_mgr_->cache_db_.expectCachedInLocationWithoutRemove(
                  file_path_, network_mgr_->repo_id_, file_oid_,
                  file_location_)) {
                fastForward();
                return;
            }
        }

        // setup prefetch task
        GetFileDownloadRequest *prefetch_task = new GetFileDownloadRequest(
            network_mgr_->account_, network_mgr_->repo_id_, parent_dir_ + file_name_);
        connect(prefetch_task, SIGNAL(success(const QString&, const QString&)),
                this, SLOT(onPrefetchFinished(const QString&, const QString&)));
        connect(prefetch_task, SIGNAL(failed(const ApiError&)),
                this, SLOT(onAborted()));
        prefetch_task->setParent(this);
        prefetch_task->send();

    }
}

void FileNetworkTask::setNetworkTask(SeafileNetworkTask *network_task)
{
    // if it is a dummy task, skip
    if (network_task == NULL)
        return;
    network_task_ = network_task;

    connect(this, SIGNAL(resume()), network_task_, SIGNAL(start()));
    connect(this, SIGNAL(cancel()), network_task_, SIGNAL(cancel()));

    connect(network_task_, SIGNAL(started()), this, SLOT(onStarted()));
    connect(network_task_, SIGNAL(updateProgress(qint64, qint64)),
            this, SLOT(onUpdateProgress(qint64, qint64)));
    connect(network_task_, SIGNAL(aborted()), this, SLOT(onAborted()));
    connect(network_task_, SIGNAL(finished()), this, SLOT(onFinished()));
}

void FileNetworkTask::run()
{
    if (status_ != SEAFILE_NETWORK_TASK_STATUS_FRESH)
        return;
    emit start();
}

void FileNetworkTask::onCancel()
{
    status_ = SEAFILE_NETWORK_TASK_STATUS_CANCELING;
    emit cancel();
}

void FileNetworkTask::onStarted()
{
    status_ = SEAFILE_NETWORK_TASK_STATUS_FRESH;
}

void FileNetworkTask::fastForward()
{
    // copy attributes from cached file
    QFileInfo cached_info_ = QFileInfo(file_location_);
    file_location_ = file_location_;
    total_bytes_ = cached_info_.size();
    processed_bytes_ = total_bytes_;
    // fast-forward progress
    emit started();
    emit updateProgress(total_bytes_ - 1, total_bytes_);
    emit finished();
    status_ = SEAFILE_NETWORK_TASK_STATUS_FINISHED;
}

void FileNetworkTask::onPrefetchFinished(const QString &url, const QString &oid)
{
    //if task is not clean, for example cancelled
    if (status_ != SEAFILE_NETWORK_TASK_STATUS_FRESH)
        return;
    //if task is clean
    if (type_ ==  SEAFILE_NETWORK_TASK_UPLOAD) {
        SeafileUploadTask *network_task = new SeafileUploadTask(
          network_mgr_->account_.token, url, parent_dir_, file_name_, file_location_);
        setNetworkTask(network_task);

        network_task->moveToThread(network_mgr_->worker_thread_);
        connect(network_mgr_->worker_thread_, SIGNAL(finished()), this, SLOT(onCancel()));
        network_mgr_->startThread();
        // begin network task
        emit resume();
    } else if (type_ ==  SEAFILE_NETWORK_TASK_DOWNLOAD) {
        if (!oid.isEmpty())
            file_oid_ = oid;

        // if hit the cache, remove if the cached item is inaccurate
        if (network_mgr_->cache_db_.expectCachedInLocation(
                file_path_, network_mgr_->repo_id_, file_oid_,
                file_location_)) {
            fastForward();
            return;
        }
        // if cache missed, create a network task
        SeafileDownloadTask *network_task = new SeafileDownloadTask(network_mgr_->account_.token, url, file_name_, file_location_);
        setNetworkTask(network_task);
        network_task->moveToThread(network_mgr_->worker_thread_);
        connect(network_mgr_->worker_thread_, SIGNAL(finished()), this, SLOT(onCancel()));
        network_mgr_->startThread();
        // begin network task
        emit resume();
    }
}

void FileNetworkTask::onAborted()
{
    status_ = SEAFILE_NETWORK_TASK_STATUS_ERROR;
    network_task_ = NULL;
    emit aborted();
}

void FileNetworkTask::onFinished()
{
    if (type_ == SEAFILE_NETWORK_TASK_DOWNLOAD)
        // check file_oid_.isEmpty internally
        network_mgr_->cache_db_.set(file_path_,
                                 network_mgr_->repo_id_,
                                 file_oid_);
    status_ = SEAFILE_NETWORK_TASK_STATUS_FINISHED;
    network_task_ = NULL;
    emit finished();
}

FileNetworkManager::FileNetworkManager(const Account &account,
                                       const QString &repo_id)
    : account_(account), repo_id_(repo_id),
    file_cache_dir_(defaultFileCachePath()),
    file_cache_path_(defaultFileCachePath()),
    cache_db_(FileCacheManager::getInstance())
{
    if (!file_cache_path_.endsWith("/"))
        file_cache_path_.append('/');
    worker_thread_ = new QThread;
    cache_db_.open();
}

FileNetworkManager::~FileNetworkManager()
{
    worker_thread_->quit();
    worker_thread_->wait();
    delete worker_thread_;
}

void FileNetworkManager::startThread()
{
    worker_thread_->start();
}

FileNetworkTask* FileNetworkManager::createDownloadTask(const QString &file_name,
                                                        const QString &parent_dir,
                                                        const QString &file_oid)
{
    file_cache_dir_.mkpath(file_cache_dir_.absoluteFilePath(repo_id_ + parent_dir));

    QString target_file_location(
        file_cache_dir_.absoluteFilePath(repo_id_ + parent_dir + file_name));

    FileNetworkTask *ftask =
        new FileNetworkTask(SEAFILE_NETWORK_TASK_DOWNLOAD, this,
                            file_name, parent_dir, target_file_location, file_oid);
    ftask->setParent(this);

    ftask->run();
    return ftask;
}


FileNetworkTask* FileNetworkManager::createUploadTask(const QString &file_name,
                                                      const QString &parent_dir,
                                                      const QString &source_file_location)
{
    FileNetworkTask *ftask =
        new FileNetworkTask(SEAFILE_NETWORK_TASK_UPLOAD, this,
                          file_name, parent_dir, source_file_location);
    ftask->setParent(this);

    ftask->run();
    return ftask;
}

void FileNetworkManager::onTaskStarted()
{
    FileNetworkTask* task = static_cast<FileNetworkTask*>(sender());
    running_tasks_.push_back(task);

    if (running_tasks_.size() == 1)
        emit taskStarted(task);
}

void FileNetworkManager::onTaskFinished()
{
    FileNetworkTask* task = static_cast<FileNetworkTask*>(sender());
    const int count = running_tasks_.indexOf(task);
    // this task is in the running list, remove it
    if (count >= 0)
        running_tasks_.removeOne(task);
    // this task is at the first place, replace it with next one
    if (count == 0 && !running_tasks_.isEmpty())
        emit taskStarted(running_tasks_.first());
}
