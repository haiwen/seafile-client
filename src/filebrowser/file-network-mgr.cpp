#include "file-network-mgr.h"

#include <QThread>
#include <QDebug>
#include "utils/utils.h"
#include "api/api-error.h"
#include "network/task.h"

FileNetworkManager::FileNetworkManager(const Account &account)
    : account_(account),
    file_cache_dir_(defaultFileCachePath()),
    file_cache_path_(defaultFileCachePath())
{
    if (!file_cache_path_.endsWith("/"))
        file_cache_path_.append('/');
    worker_thread_ = new QThread;
}

FileNetworkManager::~FileNetworkManager()
{
    worker_thread_->quit();
    worker_thread_->wait();
    delete worker_thread_;
}

FileNetworkTask* FileNetworkManager::createDownloadTask(const QString &repo_id,
                                           const QString &path,
                                           const QString &file_name,
                                           const QString &oid)
{
    file_cache_dir_.mkpath(file_cache_dir_.absoluteFilePath(repo_id + path));

    QString file_location(
        file_cache_dir_.absoluteFilePath(repo_id + path + file_name));

    if (!oid.isEmpty()) {
        FileNetworkTask *associated_task;
        if (cached_tasks_.contains(oid) &&
            (associated_task = cached_tasks_[oid]) &&
            associated_task->oid() == oid &&
            associated_task->status() == SEAFILE_NETWORK_TASK_STATUS_FINISHED &&
            QFileInfo(associated_task->fileLocation()).exists()) {
            //TODO : duplicate task and copy file here
            return associated_task;
        }
        cached_tasks_.remove(oid);
    }

    // attempt to remove conflicting file
    if (QFileInfo(file_location).isFile())
        QFile::remove(file_location);

    SeafileDownloadTask *network_task =
        network_task_builder_.createDownloadTask(account_, repo_id, path,
                                                 file_name, file_location);
    FileNetworkTask *ftask =
        new FileNetworkTask(SEAFILE_NETWORK_TASK_DOWNLOAD, network_task, this,
                            repo_id, path, file_name, file_location);
    ftask->setParent(this);

    network_task->moveToThread(worker_thread_);
    connect(worker_thread_, SIGNAL(finished()), ftask, SLOT(onCancel()));

    return ftask;
}


FileNetworkTask* FileNetworkManager::createUploadTask(const QString &repo_id,
                                           const QString &path,
                                           const QString &file_name,
                                           const QString &update_file_path)
{
    QString file_location(QFileInfo(update_file_path).absoluteFilePath());

    SeafileUploadTask* network_task = \
      network_task_builder_.createUploadTask(account_, repo_id,
                                  path, file_name, file_location);

    FileNetworkTask* ftask = \
        new FileNetworkTask(SEAFILE_NETWORK_TASK_UPLOAD, network_task, this,
                           repo_id, path, file_name, file_location);
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
