#ifndef SEAFILE_CLIENT_FILE_BROWSER_TRANSFER_MANAGER_H
#define SEAFILE_CLIENT_FILE_BROWSER_TRANSFER_MANAGER_H

#include <QObject>
#include <QQueue>
#include <QSharedPointer>

#include "api/api-error.h"
#include "seaf-dirent.h"
#include "utils/singleton.h"


template<typename Key> class QQueue;

class QThread;

class Account;
class SeafileApiRequest;
class GetDirentsRequest;
class DirentsCache;
class FileCache;
class FileUploadTask;
class FileDownloadTask;
class FileNetworkTask;

struct FileDetailInfo;
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
    bool isValid() const {
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
    TransferManager();
    ~TransferManager();

    FileDownloadTask* addDownloadTask(const Account& account,
                                      const QString& repo_id,
                                      const QString& path,
                                      const QString& local_path,
                                      bool is_save_as_task = false);

    FileDownloadTask* getDownloadTask(const QString& repo_id,
                                      const QString& path);

    void cancelDownload(const QString& repo_id, const QString& path);
    void cancelAllDownloadTasks();

    /**
     * Return all download tasks for files in the `parent_dir`
     */
    QList<FileDownloadTask*> getDownloadTasks(const QString& repo_id,
                                              const QString& parent_dir);

    void addUploadTask(const QString& repo_id,
                       const QString& repo_parent_path,
                       const QString& local_path,
                       const QString& name,
                       int folder_task_id = 0);

private slots:
    void onDownloadTaskFinished(bool success);
    void onUploadTaskFinished(bool success);

    void onGetFileDetailSuccess(const FileDetailInfo&);
    void onGetFileDetailFailed(const ApiError&);

private:
    bool isValidTask(const FileTaskRecord *task);

    void startDownloadTask(const QSharedPointer<FileDownloadTask> &task);
    QSharedPointer<FileDownloadTask> current_download_;
    QQueue<QSharedPointer<FileDownloadTask> > pending_downloads_;

    void checkUploadName(const FileTaskRecord *params);
    void startUploadTask(const FileTaskRecord *params);
    QSharedPointer<FileUploadTask> current_upload_;
    FileTaskRecord current_upload_task_;
    bool upload_checking_ = false;
    bool use_upload_ = true;

    QString account_signature_;

};


#endif // SEAFILE_CLIENT_FILE_BROWSER_TRANSFER_MANAGER_H
