#ifndef SEAFILE_CLIENT_FILE_BROWSER_DATA_MANAGER_H
#define SEAFILE_CLIENT_FILE_BROWSER_DATA_MANAGER_H

#include <QObject>
#include <QHash>

#include "api/api-error.h"
#include "account.h"
#include "seaf-dirent.h"
#include "utils/singleton.h"


template<typename Key> class QList;

class SeafileApiRequest;
class GetDirentsRequest;
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

    void renameDirent(const QString &repo_id,
                      const QString &path,
                      const QString &new_path,
                      bool is_file = true);

    void removeDirent(const QString &repo_id,
                      const QString &path,
                      bool is_file = true);

    void shareDirent(const QString &repo_id,
                     const QString &path,
                     bool is_file = true);

    QString getLocalCachedFile(const QString& repo_id,
                               const QString& path,
                               const QString& file_id);

    FileDownloadTask *createDownloadTask(const QString& repo_id,
                                         const QString& path);

    FileUploadTask* createUploadTask(const QString& repo_id,
                                     const QString& path,
                                     const QString& local_path);

    bool isRepoPasswordSet(const QString& repo_id) const;
    void setRepoPasswordSet(const QString& repo_id);

    QString getRepoCacheFolder(const QString& repo_id) const;

signals:
    void getDirentsSuccess(const QList<SeafDirent>& dirents);
    void getDirentsFailed(const ApiError& error);

    void renameDirentSuccess();
    void renameDirentFailed(const ApiError& error);

    void removeDirentSuccess();
    void removeDirentFailed(const ApiError& error);

    void shareDirentSuccess(const QString& link);
    void shareDirentFailed(const ApiError& error);

private slots:
    void onGetDirentsSuccess(const QList<SeafDirent>& dirents);
    void onFileDownloadFinished(bool success);

    void onRenameDirentSuccess();
    void onRemoveDirentSuccess();

private:
    QString getLocalCacheFilePath(const QString& repo_id,
                                  const QString& path);
    const Account account_;

    GetDirentsRequest *get_dirents_req_;

    QList<SeafileApiRequest*> reqs_;

    FileCacheDB *filecache_db_;

    DirentsCache *dirents_cache_;

    static QHash<QString, qint64> passwords_cache_;
};


#endif // SEAFILE_CLIENT_FILE_BROWSER_DATA_MANAGER_H
