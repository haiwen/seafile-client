#include "file-network-mgr.h"

#include <QThread>
#include <QDebug>
#include "account.h"
#include "utils/utils.h"
#include "api/api-error.h"
#include "network/task.h"

FileNetworkTask::FileNetworkTask(const SeafileNetworkTaskType type,
    SeafileNetworkTask *network_task, FileNetworkManager *network_mgr,
    const QString &repo_id, const QString &path,
    const QString &file_name, const QString &file_location)
    :   repo_id_(repo_id), path_(path), file_name_(file_name),
        file_location_(file_location), processed_bytes_(0), total_bytes_(0),
        status_(SEAFILE_NETWORK_TASK_STATUS_FRESH),
        type_(type), network_task_(network_task), network_mgr_(network_mgr)
    {
        if (network_task_ == NULL)
            return;

        connect(this, SIGNAL(start()), network_task, SIGNAL(start()));
        connect(this, SIGNAL(resume()), network_task, SIGNAL(resume()));
        connect(this, SIGNAL(cancel()), network_task, SIGNAL(cancel()));

        connect(network_task, SIGNAL(prefetchFinished()),
                this, SLOT(onPrefetchFinished()));

        connect(network_task, SIGNAL(started()), this, SLOT(onStarted()));
        connect(network_task, SIGNAL(updateProgress(qint64, qint64)),
                this, SLOT(onUpdateProgress(qint64, qint64)));
        connect(network_task, SIGNAL(aborted()), this, SLOT(onAborted()));
        connect(network_task, SIGNAL(finished()), this, SLOT(onFinished()));

        if (type == SEAFILE_NETWORK_TASK_DOWNLOAD) {
             connect(network_task, SIGNAL(prefetchOid(const QString &)),
                     this, SLOT(onPrefetchOid(const QString &)));
             connect(network_task, SIGNAL(fileLocationChanged(const QString &)),
                     this, SLOT(onFileLocationChanged(const QString &)));
        }
    }

void FileNetworkTask::onRun()
{
    if (network_task_ !=NULL)
        emit start();
    else
        onFastForwardProgress();
}

void FileNetworkTask::onCancel()
{
    status_ = SEAFILE_NETWORK_TASK_STATUS_CANCELING;
    emit cancel();
}

void FileNetworkTask::onStarted()
{
    status_ = SEAFILE_NETWORK_TASK_STATUS_PROCESSING;
    emit started();
}

void FileNetworkTask::onFileLocationChanged(const QString &file_location)
{
    file_location_ = file_location;
}

void FileNetworkTask::onPrefetchOid(const QString &oid)
{
    oid_ = oid;
    emit prefetchOid(oid_);
}

void FileNetworkTask::onFastForwardProgress()
{
    emit started();
    emit updateProgress(total_bytes_ - 1, total_bytes_);
    emit finished();
    status_ = SEAFILE_NETWORK_TASK_STATUS_FINISHED;
}

void FileNetworkTask::onPrefetchFinished()
{
    if (type_ ==  SEAFILE_NETWORK_TASK_UPLOAD)
        emit resume();
    if (type_ ==  SEAFILE_NETWORK_TASK_DOWNLOAD) {
        QString cached_location = network_mgr_->cache_mgr_.get(oid_);
        if (cached_location.isEmpty()) {
            emit resume();
            return;
        }
        // hit the cache, clean up network task
        disconnect(network_task_, 0, this, 0);
        onCancel();
        network_task_ = NULL;
        // copy attributes from cached file
        QFileInfo cached_info_ = QFileInfo(cached_location);
        file_location_ = cached_location; //TODO: copy the file
        total_bytes_ = cached_info_.size();
        processed_bytes_ = total_bytes_;
        status_ = SEAFILE_NETWORK_TASK_STATUS_FINISHED;
        // fast-forward progress dialog
        onFastForwardProgress();
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
    network_mgr_->cache_mgr_.set(oid_, file_location_, file_name_, path_,
                                 network_mgr_->account_.serverUrl.toString(),
                                 network_mgr_->repo_id_);
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
}

FileNetworkManager::~FileNetworkManager()
{
    worker_thread_->quit();
    worker_thread_->wait();
    delete worker_thread_;
}

FileNetworkTask* FileNetworkManager::createDownloadTask(const QString &path,
                                                        const QString &file_name,
                                                        const QString &oid)
{
    file_cache_dir_.mkpath(file_cache_dir_.absoluteFilePath(repo_id_ + path));

    QString file_location(
        file_cache_dir_.absoluteFilePath(repo_id_ + path + file_name));

    if (!oid.isEmpty()) {
        QString cached_location = cache_mgr_.get(oid);
        if (!cached_location.isEmpty()) {
            FileNetworkTask *ctask =
                new FileNetworkTask(SEAFILE_NETWORK_TASK_DOWNLOAD, NULL, this,
                                    repo_id_, path, file_name, cached_location);
            ctask->setParent(this);
            //TODO : duplicate task and copy file here
            return ctask;
        }
    }

    // then create a download task
    // attempt to remove conflicting file
    if (QFileInfo(file_location).isFile())
        QFile::remove(file_location);

    SeafileDownloadTask *network_task =
        network_task_builder_.createDownloadTask(account_, repo_id_, path,
                                                 file_name, file_location);
    FileNetworkTask *ftask =
        new FileNetworkTask(SEAFILE_NETWORK_TASK_DOWNLOAD, network_task, this,
                            repo_id_, path, file_name, file_location);
    ftask->setParent(this);

    network_task->moveToThread(worker_thread_);
    connect(worker_thread_, SIGNAL(finished()), ftask, SLOT(onCancel()));

    return ftask;
}


FileNetworkTask* FileNetworkManager::createUploadTask(const QString &path,
                                                      const QString &file_name,
                                                      const QString &update_file_path)
{
    QString file_location(QFileInfo(update_file_path).absoluteFilePath());

    SeafileUploadTask* network_task = \
      network_task_builder_.createUploadTask(account_, repo_id_,
                                  path, file_name, file_location);

    FileNetworkTask* ftask = \
        new FileNetworkTask(SEAFILE_NETWORK_TASK_UPLOAD, network_task, this,
                           repo_id_, path, file_name, file_location);
    ftask->setParent(this);

    network_task->moveToThread(worker_thread_);
    connect(worker_thread_, SIGNAL(finished()), ftask, SLOT(onCancel()));

    return ftask;
}

void FileNetworkManager::runTask(FileNetworkTask* task)
{
    worker_thread_->start();
    // shot once
    connect(this, SIGNAL(run()), task, SLOT(onRun()));
    emit run();
    disconnect(this, SIGNAL(run()), task, SLOT(onRun()));
}
