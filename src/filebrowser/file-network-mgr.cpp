#include "file-network-mgr.h"

#include <QThread>
#include <QTimer>
#include <QDebug>

#include "account.h"
#include "utils/utils.h"
#include "api/api-error.h"
#include "network/task.h"
#include "file-browser-requests.h"
#include "file-network-mono.h"

namespace {
FileNetworkMono* mono = FileNetworkMono::getInstance();
}

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
    connect(this, SIGNAL(started()), network_mgr_ ,SLOT(networkTaskBegin()));
    connect(this, SIGNAL(aborted()), network_mgr_ ,SLOT(networkTaskEnd()));
    connect(this, SIGNAL(finished()), network_mgr_ ,SLOT(networkTaskEnd()));
    connect(this, SIGNAL(updateProgress(qint64, qint64)),
            network_mgr_ ,SLOT(networkTaskUpdate(qint64, qint64)));

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
            if (network_mgr_->cache_mgr_.expectCachedInLocationWithoutRemove(
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

void FileNetworkTask::runThis()
{
    if (status_ != SEAFILE_NETWORK_TASK_STATUS_FRESH)
        return;
    emit start();
}

void FileNetworkTask::cancelThis()
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
    status_ = SEAFILE_NETWORK_TASK_STATUS_FINISHED;

    // fast-forward progress
    emit started();
    emit updateProgress(total_bytes_ - 1, total_bytes_);
    emit finished();
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
        connect(network_mgr_->worker_thread_, SIGNAL(finished()),
                this, SLOT(cancelThis()));
        network_mgr_->worker_thread_->start();
        // begin network task
        emit resume();
    } else if (type_ ==  SEAFILE_NETWORK_TASK_DOWNLOAD) {
        if (!oid.isEmpty())
            file_oid_ = oid;

        // if hit the cache, remove if the cached item is inaccurate
        if (network_mgr_->cache_mgr_.expectCachedInLocation(
                file_path_, network_mgr_->repo_id_, file_oid_,
                file_location_)) {
            fastForward();
            return;
        }
        // if cache missed, create a network task
        SeafileDownloadTask *network_task = new SeafileDownloadTask(network_mgr_->account_.token, url, file_name_, file_location_);
        setNetworkTask(network_task);
        network_task->moveToThread(network_mgr_->worker_thread_);
        connect(network_mgr_->worker_thread_, SIGNAL(finished()),
                this, SLOT(cancelThis()));
        network_mgr_->worker_thread_->start();
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
        network_mgr_->cache_mgr_.set(file_path_,
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
    cache_mgr_(FileCacheManager::getInstance())
{
    if (!file_cache_path_.endsWith("/"))
        file_cache_path_.append('/');
    worker_thread_ = new QThread;
    cache_mgr_.open();

    mono->RegisterManager(this);
}

FileNetworkManager::~FileNetworkManager()
{
    mono->UnregisterManager(this);

    cancelAll();
    worker_thread_->quit();
    worker_thread_->wait();
    delete worker_thread_;
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

    ftask->runThis();
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

    ftask->runThis();
    return ftask;
}

void FileNetworkManager::cancelUploading()
{
    foreach(FileNetworkTask* task, upload_tasks_)
    {
        task->cancelThis();
        task->deleteLater();
    }
    upload_tasks_.clear();
}

void FileNetworkManager::cancelDownloading()
{
    foreach(FileNetworkTask* task, download_tasks_)
    {
        task->cancelThis();
        task->deleteLater();
    }
    download_tasks_.clear();
}

void FileNetworkManager::networkTaskBegin()
{
    FileNetworkTask* task = static_cast<FileNetworkTask*>(sender());

    if (!task)
        return;

    if (task->type() == SEAFILE_NETWORK_TASK_UPLOAD)
        upload_tasks_.push_back(task);
    else if (task->type() == SEAFILE_NETWORK_TASK_DOWNLOAD)
        download_tasks_.push_back(task);
    else
        return;

    emit taskRegistered(task);
}

void FileNetworkManager::networkTaskEnd()
{
    FileNetworkTask* task = static_cast<FileNetworkTask*>(sender());

    if (!task)
        return;

    emit taskUnregistered(task);
}

void FileNetworkManager::networkTaskUpdate(qint64 bytes, qint64 total_bytes)
{
    // do nothing
}

