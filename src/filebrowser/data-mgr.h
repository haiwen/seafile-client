#ifndef SEAFILE_CLIENT_FILE_BROWSER_DATA_MANAGER_H
#define SEAFILE_CLIENT_FILE_BROWSER_DATA_MANAGER_H

#include <QObject>
#include <QList>

#include "api/api-error.h"
#include "account.h"
#include "seaf-dirent.h"
#include "utils/singleton.h"

class GetDirentsRequest;
template<typename Key, typename T> class QCache;

class DirentsCache;
class FileCacheDB;
class FileUploadTask;
class FileDownloadTask;

/**
 * DataManager is responsible for getting dirents/files from seahub, as well
 * as the caching policy.
 *
 */
class DataManager : public QObject {
    Q_OBJECT
public:
    DataManager(const Account& account);
    ~DataManager();

    bool getDirents(const QString& repo_id,
                    const QString& path,
                    QList<SeafDirent> *dirents);

    void getDirentsFromServer(const QString& repo_id,
                              const QString& path);

    QString getLocalCachedFile(const QString& repo_id,
                               const QString& path,
                               const QString& file_id);

    FileDownloadTask *createDownloadTask(const QString& repo_id,
                                         const QString& path);

    FileUploadTask* createUploadTask(const QString& repo_id,
                                     const QString& path,
                                     const QString& local_path);


signals:
    void getDirentsSuccess(const QList<SeafDirent>& dirents);
    void getDirentsFailed(const ApiError& error);

private slots:
    void onGetDirentsSuccess(const QList<SeafDirent>& dirents);
    void onFileDownloadFinished(bool success);

private:
    const Account account_;

    GetDirentsRequest *get_dirents_req_;

    FileCacheDB *filecache_db_;

    DirentsCache *dirents_cache_;
};


#endif // SEAFILE_CLIENT_FILE_BROWSER_DATA_MANAGER_H
