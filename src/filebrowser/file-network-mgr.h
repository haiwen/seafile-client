#ifndef SEAFILE_CLIENT_FILE_BROWSER_NETWORK_MANAGER_H
#define SEAFILE_CLIENT_FILE_BROWSER_NETWORK_MANAGER_H

#include <QDir>

#include "network/task-builder.h"
#include "network/task.h"
#include "account.h"
#include "file-cache-mgr.h"

class ApiError;
class QThread;
class FileNetworkManager;
class FileNetworkTask : public QObject {
    Q_OBJECT;

    QString repo_id_;
    QString path_;
    QString file_name_;
    QString file_location_;
    QString oid_;
    qint64 processed_bytes_;
    qint64 total_bytes_;
    SeafileNetworkTaskStatus status_;
    SeafileNetworkTaskType type_;
    SeafileNetworkTask *network_task_;
    FileNetworkManager *network_mgr_;

    inline void onFastForwardProgress();

signals:
    void start();
    void cancel();
    void resume();

    void started();
    void updateProgress(qint64 processed_bytes, qint64 total_bytes);
    void prefetchOid(const QString &oid);
    void aborted();
    void finished();
private slots:
    inline void onRun();
    inline void onCancel();

    inline void onStarted();
    inline void onUpdateProgress(qint64 processed_bytes, qint64 total_bytes);
    inline void onFileLocationChanged(const QString &file_location);
    inline void onPrefetchOid(const QString &oid);
    inline void onPrefetchFinished();
    inline void onAborted();
    inline void onFinished();

public:
    FileNetworkTask(const SeafileNetworkTaskType type,
                    SeafileNetworkTask *network_task,
                    FileNetworkManager *network_mgr,
                    const QString &repo_id,
                    const QString &path,
                    const QString &file_name,
                    const QString &file_location)
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

    QString path() const { return path_; }
    QString fileName() const { return file_name_; }
    QString fileLocation() const { return file_location_; }

    QString oid() const { return oid_; }
    void setOid(const QString &oid) { oid_ = oid; }

    qint64 processedBytes() const { return processed_bytes_; }
    qint64 totalBytes() const { return total_bytes_; }
    SeafileNetworkTaskStatus status() const { return status_; }
    SeafileNetworkTaskType type() const { return type_; }
    FileNetworkManager* networkMgr() const { return network_mgr_; }
};

class FileNetworkManager : public QObject {
    Q_OBJECT
    friend class FileNetworkTask;
public:
    FileNetworkManager(const Account &account, const QString &repo_id);
    ~FileNetworkManager();

    FileNetworkTask* createDownloadTask(const QString &path,
                                        const QString &file_name,
                                        const QString &oid = QString());

    FileNetworkTask* createUploadTask(const QString &path,
                                      const QString &file_name,
                                      const QString &upload_file_path);

    void runTask(FileNetworkTask* task);

signals:
    void run();

private:
    int addTask(FileNetworkTask* task);

    const Account account_;
    const QString repo_id_;

    QDir file_cache_dir_;
    QString file_cache_path_;

    SeafileNetworkTaskBuilder network_task_builder_;

    QThread *worker_thread_;

    FileCacheManager &cache_mgr_;
};

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

void FileNetworkTask::onUpdateProgress(qint64 processed_bytes,
                                            qint64 total_bytes)
{
    if (processed_bytes_++ != 0) //processed_bytes contains network cache
        processed_bytes_ = processed_bytes;
    total_bytes_ = total_bytes;
    emit updateProgress(processed_bytes_, total_bytes_);
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

#endif //SEAFILE_CLIENT_FILE_BROWSER_NETWORK_MANAGER_H
