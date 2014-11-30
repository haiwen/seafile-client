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
class FileCacheDB;
class FileUploadTask;
class FileDownloadTask;
class FileNetworkTask;

/**
 * TransferManager manages all upload/download tasks.
 *
 */
class TransferManager : public QObject {
    SINGLETON_DEFINE(TransferManager)
    Q_OBJECT
public:
    TransferManager();
    ~TransferManager();

    FileDownloadTask *addDownloadTask(const Account& account,
                                      const QString& repo_id,
                                      const QString& path,
                                      const QString& local_path);

private slots:
    void onFileDownloadFinished(bool success);

private:
    QQueue<FileDownloadTask *> download_tasks_;

    static QThread *worker_thread_;

    FileDownloadTask *current_task_;
};


#endif // SEAFILE_CLIENT_FILE_BROWSER_TRANSFER_MANAGER_H
