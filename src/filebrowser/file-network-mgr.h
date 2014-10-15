#ifndef SEAFILE_CLIENT_FILE_BROWSER_NETWORK_MANAGER_H
#define SEAFILE_CLIENT_FILE_BROWSER_NETWORK_MANAGER_H

#include <QList>
#include <QDir>

#include "network/task.h"
#include "file-cache-mgr.h"

class Account;
class ApiError;
class QThread;
class QTimer;
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

signals:
    // emitted once a task starts
    void taskRegistered(const FileNetworkTask* task);

    // emitted once a task ends
    void taskUnregistered(const FileNetworkTask* task);

    // emitted per 100ms
    void taskProgressed(qint64 bytes, qint64 total_bytes);

    // emitted per 100ms
    void taskRateBeat(qint64 rate);

public slots:
    // cancel all underlying tasks
    void cancelAll();

private slots:
    // receive task begin signal
    void networkTaskBegin();

    // receive task end signal
    void networkTaskEnd();

    // receive task update signal, dummy currently
    void networkTaskUpdate(qint64 bytes, qint64 total_bytes);

    // triggered by a timer
    void networkTaskBeat();

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
    QList<FileNetworkTask*> tasks_;

    // Timer
    QTimer *timer_;
};

#endif //SEAFILE_CLIENT_FILE_BROWSER_NETWORK_MANAGER_H
