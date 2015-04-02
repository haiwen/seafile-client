#ifndef SEAFILE_CLIENT_FILE_BROWSER_DATA_MANAGER_H
#define SEAFILE_CLIENT_FILE_BROWSER_DATA_MANAGER_H

#include <QObject>
#include <QHash>
#include <QScopedPointer>

#include "api/api-error.h"
#include "account.h"
#include "seaf-dirent.h"
#include "utils/singleton.h"


template<typename Key> class QList;

class SeafileApiRequest;
class GetDirentsRequest;
class GetRepoRequest;
class CreateSubrepoRequest;
class DirentsCache;
class FileCacheDB;
class FileUploadTask;
class FileDownloadTask;
class SyncedSubfolder;

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

    void createDirectory(const QString &repo_id,
                         const QString &path);

    void renameDirent(const QString &repo_id,
                      const QString &path,
                      const QString &new_path,
                      bool is_file);

    void removeDirent(const QString &repo_id,
                      const QString &path,
                      bool is_file);

    void shareDirent(const QString &repo_id,
                     const QString &path,
                     bool is_file);

    void copyDirents(const QString &repo_id,
                     const QString &dir_path,
                     const QStringList &file_names,
                     const QString &dst_repo_id,
                     const QString &dst_dir_path);

    void moveDirents(const QString &repo_id,
                     const QString &dir_path,
                     const QStringList &file_names,
                     const QString &dst_repo_id,
                     const QString &dst_dir_path);

    QString getLocalCachedFile(const QString& repo_id,
                               const QString& path,
                               const QString& file_id);

    FileDownloadTask* createDownloadTask(const QString& repo_id,
                                         const QString& path);

    FileUploadTask* createUploadTask(const QString& repo_id,
                                     const QString& parent_dir,
                                     const QString& local_path,
                                     const QString& name,
                                     const bool overwrite);

    FileUploadTask* createUploadMultipleTask(const QString& repo_id,
                                             const QString& parent_dir,
                                             const QString& local_path,
                                             const QStringList& names,
                                             const bool overwrite);

    bool isRepoPasswordSet(const QString& repo_id) const;
    void setRepoPasswordSet(const QString& repo_id);

    QString getRepoCacheFolder(const QString& repo_id) const;

    static QString getLocalCacheFilePath(const QString& repo_id,
                                         const QString& path);

    void createSubrepo(const QString &name, const QString& repo_id, const QString &path, const QString &password);

signals:
    void getDirentsSuccess(const QList<SeafDirent>& dirents);
    void getDirentsFailed(const ApiError& error);

    void createDirectorySuccess(const QString& path);
    void createDirectoryFailed(const ApiError& error);

    void renameDirentSuccess(const QString& path, const QString& new_name);
    void renameDirentFailed(const ApiError& error);

    void removeDirentSuccess(const QString& path);
    void removeDirentFailed(const ApiError& error);

    void shareDirentSuccess(const QString& link);
    void shareDirentFailed(const ApiError& error);

    void copyDirentsSuccess();
    void copyDirentsFailed(const ApiError& error);

    void moveDirentsSuccess();
    void moveDirentsFailed(const ApiError& error);

    void createSubrepoSuccess(const ServerRepo &repo);
    void createSubrepoFailed(const ApiError& error);

private slots:
    void onGetDirentsSuccess(const QList<SeafDirent>& dirents);
    void onFileUploadFinished(bool success);
    void onFileDownloadFinished(bool success);

    void onCreateDirectorySuccess();
    void onRenameDirentSuccess();
    void onRemoveDirentSuccess();
    void onCopyDirentsSuccess();
    void onMoveDirentsSuccess();

    void onCreateSubrepoSuccess(const QString& new_repoid);
    void onCreateSubrepoRefreshSuccess(const ServerRepo& new_repo);

private:
    void removeDirentsCache(const QString& repo_id,
                            const QString& path,
                            bool is_file);
    const Account account_;

    QScopedPointer<GetDirentsRequest> get_dirents_req_;

    QScopedPointer<CreateSubrepoRequest> create_subrepo_req_;
    QString create_subrepo_parent_repo_id_;
    QString create_subrepo_parent_path_;
    QScopedPointer<GetRepoRequest> get_repo_req_;

    QList<SeafileApiRequest*> reqs_;

    FileCacheDB *filecache_db_;

    DirentsCache *dirents_cache_;

    static QHash<QString, qint64> passwords_cache_;
};


#endif // SEAFILE_CLIENT_FILE_BROWSER_DATA_MANAGER_H
