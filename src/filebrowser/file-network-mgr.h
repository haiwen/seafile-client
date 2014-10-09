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

    // file name
    QString file_name_;
    // file parent_dir
    QString parent_dir_;
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

    void setNetworkTask(SeafileNetworkTask *task);

    // if hit the cache
    void fastForward(const QString &cached_location);

signals:
    // networktask slots are connected with these signals
    // dummy
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

private slots:
    // command order slot, connected with high level stuff
    void onCancel();

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

public:
    //entry point
    void run();

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

    // start the underlying thread!
    void startThread();
public:
    FileNetworkManager(const Account &account, const QString &repo_id);
    ~FileNetworkManager();

    FileNetworkTask* createDownloadTask(const QString &file_name,
                                        const QString &parent_dir,
                                        const QString &oid = QString());

    FileNetworkTask* createUploadTask(const QString &file_name,
                                      const QString &parent_dir,
                                      const QString &source_file_location);

signals:
    //signal for progress dialog
    void taskStarted(const FileNetworkTask* current_task);

private slots:
    //connected with filenetworktask once it created
    void onTaskStarted();

    //connected with filenetworktask whether it succeeds or fails
    void onTaskFinished();

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
    QList<FileNetworkTask*> running_tasks_;
};

#endif //SEAFILE_CLIENT_FILE_BROWSER_NETWORK_MANAGER_H
