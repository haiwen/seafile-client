#include "file-network-mgr.h"

#include <QThread>
#include <QDebug>
#include "utils/utils.h"
#include "api/api-error.h"
#include "network/task.h"

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
