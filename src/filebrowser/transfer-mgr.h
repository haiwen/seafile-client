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
class TransferProgress;

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

    void getTransferRate(uint *upload_rate, uint *download_rate);

private slots:
    void onDownloadTaskFinished(bool success);
    void refreshRate();

private:
    void startDownloadTask(const QSharedPointer<FileDownloadTask> &task);

    QSharedPointer<FileDownloadTask> current_download_;
    QQueue<QSharedPointer<FileDownloadTask> > pending_downloads_;

    QTimer *refresh_rate_timer_;
    TransferProgress *download_progress_;
    TransferProgress *upload_progress_;
};


#endif // SEAFILE_CLIENT_FILE_BROWSER_TRANSFER_MANAGER_H
