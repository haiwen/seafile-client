#ifndef SEAFILE_CLIENT_FILE_BROWSER_NETWORK_MANAGER_H
#define SEAFILE_CLIENT_FILE_BROWSER_NETWORK_MANAGER_H

#include <QList>
#include <QDir>

#include "network/task.h"
#include "file-cache-mgr.h"

class Account;
class ApiError;
class QThread;
class FileNetworkManager;

class FileNetworkTask : public QObject {
    Q_OBJECT
public:
    FileNetworkTask(const SeafileNetworkTaskType type,
                    FileNetworkManager *network_mgr,
                    const QString &file_name,
                    const QString &parent_dir,
                    const QString &file_location,
                    const QString &file_oid = QString());

    QString parentDir() const { return parent_dir_; }
    QString fileOid() const { return file_oid_; }
    QString fileName() const { return file_name_; }
    QString fileLocation() const { return file_location_; }

    qint64 processedBytes() const { return processed_bytes_; }
    qint64 totalBytes() const { return total_bytes_; }
    SeafileNetworkTaskStatus status() const { return status_; }
    SeafileNetworkTaskType type() const { return type_; }

signals:
    // networktask slots are connected with these signals
    void start();
    // command network task to abort if any
    void cancel();
    // command network task to start if any
    void resume();

    //status changed signal
    void started();
    void updateProgress(qint64 processed_bytes, qint64 total_bytes);
    void aborted();
    void finished();

public slots:
    // signal this task
    void runThis();

    // cancel this task
    void cancelThis();

private slots:
    // connected with self's start signal
    void onStarted();

    // connected with prefetch task
    void onPrefetchFinished(const QString &url, const QString &oid = QString());

    // connected with network task
    inline void onUpdateProgress(qint64 processed_bytes, qint64 total_bytes);

    // connected with network task and prefetch task
    void onAborted();

    // connected with network task
    void onFinished();

private:
    // set worker task
    void setNetworkTask(SeafileNetworkTask *task);

    // omit the worker task
    void fastForward();

private:
    // file name
    QString file_name_;

    // file parent_dir
    QString parent_dir_;

    // file path = parent_dir_ + file_name_
    QString file_path_;

    // target_file_location or source_file_location
    QString file_location_;

    // file object id
    QString file_oid_;

    // network task's attributes
    qint64 processed_bytes_;
    qint64 total_bytes_;

    // current task status
    SeafileNetworkTaskStatus status_;

    // current task type
    SeafileNetworkTaskType type_;

    // private, should not used outside
    SeafileNetworkTask *network_task_;
    FileNetworkManager *network_mgr_;
};

void FileNetworkTask::onUpdateProgress(qint64 processed_bytes,
                                            qint64 total_bytes)
{
    // work around with the weired progressbar behaviour and network BUF
    if (processed_bytes_++ != 0) //processed_bytes contains network cache
        processed_bytes_ = processed_bytes;
    total_bytes_ = total_bytes;

    if (processed_bytes_ >= total_bytes_)
        processed_bytes_ = total_bytes_ - 1;

    emit updateProgress(processed_bytes_, total_bytes_);
}

class FileNetworkManager : public QObject {
    Q_OBJECT
    friend class FileNetworkTask;
public:
    FileNetworkManager(const Account &account, const QString &repo_id);
    ~FileNetworkManager();

    FileNetworkTask* createDownloadTask(const QString &file_name,
                                        const QString &parent_dir,
                                        const QString &oid = QString());

    FileNetworkTask* createUploadTask(const QString &file_name,
                                      const QString &parent_dir,
                                      const QString &source_file_location);

    QList<FileNetworkTask*> &uploadTasks() { return upload_tasks_; }
    QList<FileNetworkTask*> &downloadTasks() { return download_tasks_; }

    int uploadedTaskCount() const {
        int count = 0;
        foreach (FileNetworkTask *task, upload_tasks_) {
            if (task->status() == SEAFILE_NETWORK_TASK_STATUS_FINISHED)
                count++;
        }
        return count;
    }

    int downloadedTaskCount() const {
        int count = 0;
        foreach(FileNetworkTask* task, download_tasks_) {
            if (task->status() == SEAFILE_NETWORK_TASK_STATUS_FINISHED)
                count++;
        }
        return count;
    }

    // emitted per kNetworkRateBeatInterval
    qint64 getProgressedBytes() const {
        return getUploadedBytes() + getDownloadedBytes();
    }
    qint64 getTotalProgressedBytes() const {
        return getTotalUploadedBytes() + getTotalDownloadedBytes();
    }
    qint64 getUploadedBytes() const {
        qint64 upload_bytes = 0;
        foreach(const FileNetworkTask* task, upload_tasks_)
        {
            upload_bytes += task->processedBytes();
        }
        return upload_bytes;
    }
    qint64 getTotalUploadedBytes() const {
        qint64 upload_total_bytes = 0;
        foreach(const FileNetworkTask* task, upload_tasks_)
        {
            upload_total_bytes += task->totalBytes();
        }
        return upload_total_bytes;
    }
    qint64 getDownloadedBytes() const {
        qint64 download_bytes = 0;
        foreach(const FileNetworkTask* task, download_tasks_)
        {
            download_bytes += task->processedBytes();
        }
        return download_bytes;
    }
    qint64 getTotalDownloadedBytes() const {
        qint64 download_total_bytes = 0;
        foreach(const FileNetworkTask* task, download_tasks_)
        {
            download_total_bytes += task->totalBytes();
        }
        return download_total_bytes;
    }

signals:
    // emitted once a task starts
    void taskRegistered(const FileNetworkTask* task);

    // emitted once a task ends
    void taskUnregistered(const FileNetworkTask* task);

public slots:
    // cancel all underlying tasks
    void cancelAll() { cancelUploading(); cancelDownloading(); }
    void cancelUploading();
    void cancelDownloading();

private slots:
    // receive task begin signal
    void networkTaskBegin();

    // receive task end signal
    void networkTaskEnd();

    // receive task update signal, dummy currently
    void networkTaskUpdate(qint64 bytes, qint64 total_bytes);

private:
    /* keep an reference to the copy in the caller */
    const Account& account_;

    /* keep an copy from the caller */
    const QString repo_id_;

    /* download path */
    QDir file_cache_dir_;

    /* download path, string */
    QString file_cache_path_;

    /* reference to file cache manager*/
    FileCacheManager &cache_mgr_;

    /* underlying work thread */
    QThread *worker_thread_;

    /* list of current running tasks */
    QList<FileNetworkTask*> upload_tasks_;
    QList<FileNetworkTask*> download_tasks_;
};

#endif //SEAFILE_CLIENT_FILE_BROWSER_NETWORK_MANAGER_H
