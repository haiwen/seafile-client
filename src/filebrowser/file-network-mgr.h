#ifndef SEAFILE_CLIENT_FILE_BROWSER_NETWORK_MANAGER_H
#define SEAFILE_CLIENT_FILE_BROWSER_NETWORK_MANAGER_H

#include <QDir>

#include "network/task-builder.h"
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

    void onFastForwardProgress();

signals:
    //command order signal, triggered by command order slots
    void start();
    void cancel();
    void resume();

    //status changed signal
    void started();
    void updateProgress(qint64 processed_bytes, qint64 total_bytes);
    void prefetchOid(const QString &oid);
    void aborted();
    void finished();

private slots:
    //command order slot
    void onRun();
    void onCancel();

    //status changed slot
    void onStarted();
    inline void onUpdateProgress(qint64 processed_bytes, qint64 total_bytes);
    void onFileLocationChanged(const QString &file_location);
    void onPrefetchOid(const QString &oid);
    void onPrefetchFinished();
    void onAborted();
    void onFinished();

public:
    FileNetworkTask(const SeafileNetworkTaskType type,
                    SeafileNetworkTask *network_task,
                    FileNetworkManager *network_mgr,
                    const QString &repo_id,
                    const QString &path,
                    const QString &file_name,
                    const QString &file_location);

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

    const Account& account_;
    const QString repo_id_;

    QDir file_cache_dir_;
    QString file_cache_path_;

    SeafileNetworkTaskBuilder network_task_builder_;

    QThread *worker_thread_;

    FileCacheManager &cache_mgr_;
};

#endif //SEAFILE_CLIENT_FILE_BROWSER_NETWORK_MANAGER_H
