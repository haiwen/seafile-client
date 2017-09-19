#ifndef SEAFILE_CLIENT_FILE_BROWSER_TRANSFER_MANAGER_H
#define SEAFILE_CLIENT_FILE_BROWSER_TRANSFER_MANAGER_H

#include <QObject>
#include <QQueue>
#include <QList>
#include <QSharedPointer>

#include "api/api-error.h"
#include "seaf-dirent.h"
#include "utils/singleton.h"


template<typename Key> class QQueue;

struct sqlite3;
struct FileDetailInfo;
class QThread;

class Account;
class SeafileApiRequest;
class GetDirentsRequest;
class DirentsCache;
class FileCacheDB;
class FileUploadTask;
class FileDownloadTask;
class FileNetworkTask;
class TransferProgress;

struct FileTaskRecord {
    int id = 0;
    QString account_sign;
    QString type;
    QString repo_id;
    QString path;
    QString local_path;
    QString status;
    QString fail_reason;
    int parent_task = 0;
    bool isValid() {
        if (account_sign.isEmpty() || type.isEmpty() ||
            repo_id.isEmpty()      || path.isEmpty() ||
            local_path.isEmpty()   || status.isEmpty() ||
            id == 0) {
            return false;
        } else {
            return true;
        }
    }
};

struct FolderTaskRecord {
    int id = 0;
    QString account_sign;
    QString repo_id;
    QString path;
    QString local_path;
    QString status;
    quint64 total_bytes = 0;
    quint64 finished_files_bytes = 0;
    bool isValid() {
        if (account_sign.isEmpty() || repo_id.isEmpty() ||
            path.isEmpty()         || local_path.isEmpty() ||
            status.isEmpty()       || id == 0 ||
            total_bytes == 0) {
            return false;
        } else {
            return true;
        }
    }
};

/**
 * TransferManager manages all upload/download tasks.
 *
 * There is a pending tasks queue for all download tasks. At any moment only
 * one download task is running, others are waiting in the queue.
 *
 */
class TransferManager : public QObject {
    SINGLETON_DEFINE(TransferManager)
    Q_OBJECT
public:
    int start();

    FileDownloadTask* addDownloadTask(const Account& account,
                                      const QString& repo_id,
                                      const QString& path,
                                      const QString& local_path,
                                      bool is_save_as_task = false);
    void cancelAllDownloadTasks();

    void addUploadTask(const QString& repo_id,
                       const QString& repo_parent_path,
                       const QString& local_path,
                       const QString& name,
                       int folder_task_id = 0);
    void addUploadTasks(const QString& repo_id,
                        const QString& repo_parent_path,
                        const QString& local_parent_path,
                        const QStringList& names);
    void addUploadDirectoryTask(const QString& repo_id,
                                const QString& repo_parent_path,
                                const QString& local_path,
                                const QString& name);

    void getTransferRate(uint *upload_rate, uint *download_rate);

    /**
     * Return all transferring tasks for files in the `parent_dir`
     */
    QList<FileNetworkTask*> getTransferringTasks(const QString& repo_id,
                                                 const QString& parent_dir);
    QList<const FileTaskRecord*> getPendingUploadFiles(const QString& repo_id,
                                                       const QString& parent_dir);
    QList<const FolderTaskRecord*> getUploadFolderTasks(const QString& repo_id,
                                                        const QString& parent_dir);

    bool isTransferring(const QString& repo_id,
                        const QString& path);
    void cancelTransfer(const QString& repo_id,
                        const QString& path);

    bool isValidTask(const FileTaskRecord *task);
    bool isValidTask(const FolderTaskRecord *task);

    QString getFolderTaskProgress(const QString& repo_id,
                                  const QString& path);

public slots:
    void cancelAllTasks();

signals:
    void taskFinished();
    void uploadTaskSuccess(const FileTaskRecord *task);
    void uploadTaskFailed(const FileTaskRecord *task);

private slots:
    void onDownloadTaskFinished(bool success);
    void onUploadTaskFinished(bool success = true);
    void refreshRate();
    void onCreateDirSuccess();
    void onCreateDirFailed(const ApiError&);
    void onGetFileDetailSuccess(const FileDetailInfo&);
    void onGetFileDetailFailed(const ApiError&);

private:
    enum TaskStatus {
        TASK_PENDING = 0,
        TASK_STARTED,
        TASK_ERROR
    };

    TransferManager();
    ~TransferManager();

    void startDownloadTask(const QSharedPointer<FileDownloadTask> &task);
    void checkUploadName(const FileTaskRecord *params);
    void startUploadTask(const FileTaskRecord *params);

    void updateFailReason(int task_id, const QString& error);
    void updateStatus(int task_id,
                      TaskStatus status,
                      const QString& error = QString(),
                      const QString& table = "FileTasks");
    void deleteFromDB(int task_id,
                      const QString& table = "FileTasks");

    void cancelDownload(const QString& repo_id, const QString& path);
    void cancelUpload(const QString& repo_id, const QString& path);

    int getUploadTaskId(const QString& repo_id,
                        const QString& path,
                        const QString& local_path = QString(),
                        TaskStatus status = TASK_PENDING,
                        const QString& table = "FileTasks");
    const FileTaskRecord* getPendingFileTask();
    const FileTaskRecord* getFileTaskById(int id);
    QList<const FileTaskRecord*> getFileTasksByParentId(int parent_id);
    const FolderTaskRecord* getFolderTaskById(int id);
    void getUploadTasksFromDB();

    FileUploadTask* isCurrentUploadTask(const QString& repo_id,
                                        const QString& path,
                                        const QString& local_path = QString());
    FileDownloadTask* getDownloadTask(const QString& repo_id,
                                      const QString& path);

    QSharedPointer<FileDownloadTask> current_download_;
    QQueue<QSharedPointer<FileDownloadTask> > pending_downloads_;
    QSharedPointer<FileUploadTask> current_upload_;

    struct sqlite3 *db_;

    QTimer *refresh_rate_timer_;
    TransferProgress *download_progress_;
    TransferProgress *upload_progress_;

    QString account_signature_;
    bool overwrite_upload_ = false;
    bool use_upload_ = true;
    bool upload_checking_ = false;

    QMap <int, FileTaskRecord> file_tasks_;
    QMap <int, FolderTaskRecord> folder_tasks_;
};

#endif // SEAFILE_CLIENT_FILE_BROWSER_TRANSFER_MANAGER_H
