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

    // path is used as parent_dir
    QString parent_dir_;
    // file name
    QString file_name_;
    // target_file_location or source_file_location
    QString file_location_;
    // file object id
    QString oid_;
    qint64 processed_bytes_;
    qint64 total_bytes_;
    SeafileNetworkTaskStatus status_;
    SeafileNetworkTaskType type_;
    // underlying network task
    SeafileNetworkTask *network_task_;
    FileNetworkManager *network_mgr_;

    void fastForward(const QString &cached_location);
    SeafileNetworkTask* networkTask() const { return network_task_; }
    void setNetworkTask(SeafileNetworkTask *task);

signals:
    //networktask's command order signal, should be triggered by command order slots
    // dummy
    void start();
    // command network and prefetch task
    void cancel();
    // command network task
    void resume();

    //status changed signal
    void started();
    void updateProgress(qint64 processed_bytes, qint64 total_bytes);
    void aborted();
    void finished();

private slots:
    //command order slot
    void onCancel();

    //status changed slot
    void onStarted();
    inline void onUpdateProgress(qint64 processed_bytes, qint64 total_bytes);
    void onPrefetchFinished(const QString &url, const QString &oid = QString());
    void onAborted();
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
    QString oid() const { return oid_; }
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
    void taskStarted(const FileNetworkTask* current_task); //signal for progress dialog

private slots:
    void onTaskStarted();
    void onTaskFinished();

private:
    /* keep reference of account and repo copy from FileTableModel*/
    const Account& account_;
    const QString repo_id_;

    /* download path */
    QDir file_cache_dir_;
    QString file_cache_path_;

    /* file cache reference*/
    FileCacheManager &cache_mgr_;

    /* underlying work thread */
    QThread *worker_thread_;

    /* manage running task queue */
    QList<FileNetworkTask*> running_tasks_;
};

#endif //SEAFILE_CLIENT_FILE_BROWSER_NETWORK_MANAGER_H
