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

    void fastForward(const QString &cached_location);

signals:
    //networktask's command order signal, should be triggered by command order slots
    void start();
    void cancel();
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
    void onFileLocationChanged(const QString &file_location);
    void onPrefetchFinished(const QString &url, const QString &oid = QString());
    void onAborted();
    void onFinished();

public:
    //entry point
    void run();

public:
    FileNetworkTask(const SeafileNetworkTaskType type,
                    FileNetworkManager *network_mgr,
                    const QString &repo_id,
                    const QString &path,
                    const QString &file_name,
                    const QString &file_location,
                    const QString &oid = QString());

    QString path() const { return path_; }
    QString fileName() const { return file_name_; }
    QString fileLocation() const { return file_location_; }
    void setFileLocation(const QString& file_location) { file_location_ = file_location; }

    QString oid() const { return oid_; }
    void setOid(const QString &oid) { oid_ = oid; }

    qint64 processedBytes() const { return processed_bytes_; }
    qint64 totalBytes() const { return total_bytes_; }
    SeafileNetworkTask* networkTask() const { return network_task_; }
    void setNetworkTask(SeafileNetworkTask *task);
    SeafileNetworkTaskStatus status() const { return status_; }
    SeafileNetworkTaskType type() const { return type_; }
    FileNetworkManager* networkMgr() const { return network_mgr_; }
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

    FileNetworkTask* createDownloadTask(const QString &path,
                                        const QString &file_name,
                                        const QString &oid = QString());

    FileNetworkTask* createUploadTask(const QString &path,
                                      const QString &file_name,
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
