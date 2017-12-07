#ifndef SEAFILE_CLIENT_FILE_BROWSER_TRANSFER_MANAGER_H
#define SEAFILE_CLIENT_FILE_BROWSER_TRANSFER_MANAGER_H

#include <QObject>
#include <QQueue>

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

private slots:
    void onDownloadTaskFinished(bool success);

private:
    void startDownloadTask(FileDownloadTask* task);

    FileDownloadTask* current_download_;
    QQueue<FileDownloadTask*> pending_downloads_;
};


#endif // SEAFILE_CLIENT_FILE_BROWSER_TRANSFER_MANAGER_H
