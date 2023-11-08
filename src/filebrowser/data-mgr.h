#ifndef SEAFILE_CLIENT_FILE_BROWSER_DATA_MANAGER_H
#define SEAFILE_CLIENT_FILE_BROWSER_DATA_MANAGER_H

#include <QObject>
#include <QHash>
#include <QScopedPointer>
#include <utility>

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
class FileCache;
class FileCache;
class FileNetworkTask;
class FileUploadTask;
class FileDownloadTask;
class QueryAsyncOperationProgress;
class AsyncCopyAndMoveOneItemRequest;

/**
 * DataManager is responsible for getting dirents/files from seahub, as well
 * as the caching policy.
 *
 */
class DataManager : public QObject {
    SINGLETON_DEFINE(DataManager)
    Q_OBJECT

public:
    DataManager();
    ~DataManager();

    void start();

    const Account& account() const { return account_; }

    bool getDirents(const QString& repo_id,
                    const QString& path,
                    QList<SeafDirent> *dirents,
                    bool *current_readonly);

    void getDirentsFromServer(const QString& repo_id,
                              const QString& path);

    void createDirectory(const QString &repo_id,
                         const QString &path);

    void renameDirent(const QString &repo_id,
                      const QString &path,
                      const QString &new_path,
                      bool is_file);

    void lockFile(const QString &repo_id,
                  const QString &path,
                  bool lock);

    void removeDirent(const QString &repo_id,
                      const QString &path,
                      bool is_file);

    void removeDirents(const QString &repo_id,
                       const QString &parent_path,
                       const QStringList &filenames);

    void shareDirent(const QString &repo_id,
                     const QString &path,
                     bool is_file);

    void copyDirents(const QString &repo_id,
                     const QString &dir_path,
                     const QMap<QString, int> &dict_file_names,
                     const QString &dst_repo_id,
                     const QString &dst_dir_path);

    void moveDirents(const QString &repo_id,
                     const QString &dir_path,
                     const QMap<QString, int> &dict_file_names,
                     const QString &dst_repo_id,
                     const QString &dst_dir_path);

    QString getLocalCachedFile(const QString& repo_id,
                               const QString& path,
                               const QString& file_id);

    FileDownloadTask* createDownloadTask(const QString& repo_id,
                                         const QString& path);

    FileDownloadTask* createSaveAsTask(const QString& repo_id,
                                       const QString& path,
                                       const QString& local_path);

    FileUploadTask* createUploadTask(const QString& repo_id,
                                     const QString& parent_dir,
                                     const QString& local_path,
                                     const QString& commit_id,
                                     const QString& name,
                                     const bool overwrite);

    FileUploadTask* createUploadMultipleTask(const QString& repo_id,
                                             const QString& parent_dir,
                                             const QString& local_path,
                                             const QStringList& names,
                                             const bool overwrite);

    bool isRepoPasswordSet(const QString& repo_id) const;
    QString repoPassword(const QString& repo_id) const {
        if (!isRepoPasswordSet(repo_id))
            return QString();
        return passwords_cache_[repo_id].second;
    }
    void setRepoPasswordSet(const QString& repo_id, const QString& password);

    QString getRepoCacheFolder(const QString& repo_id) const;

    static QString getLocalCacheFilePath(const QString& repo_id,
                                         const QString& path);

    void createSubrepo(const QString &name, const QString& repo_id, const QString &path);

signals:
    void aboutToDestroy();

    void getDirentsSuccess(bool current_readonly, const QList<SeafDirent>& dirents, const QString& repo_id);
    void getDirentsFailed(const ApiError& error, const QString& repo_id);

    void createDirectorySuccess(const QString& path, const QString& repo_id);
    void createDirectoryFailed(const ApiError& error);

    void lockFileSuccess(const QString& path, bool lock, const QString& repo_id);
    void lockFileFailed(const ApiError& error);

    void renameDirentSuccess(const QString& path, const QString& new_name, const QString& repo_id);
    void renameDirentFailed(const ApiError& error);

    void removeDirentSuccess(const QString& path, const QString& repo_id);
    void removeDirentFailed(const ApiError& error);

    void removeDirentsSuccess(const QString& parent_path, const QStringList& filenames, const QString& repo_id);
    void removeDirentsFailed(const ApiError& error);

    void sigShareDirentSuccess(const QString& link, const QString& repo_id, const QString& repo_path);
    void shareDirentFailed(const ApiError& error);

    void copyDirentsSuccess(const QString& dst_repo_id);
    void copyDirentsFailed(const ApiError& error);

    void moveDirentsSuccess(const QString& dst_repo_id);
    void moveDirentsFailed(const ApiError& error);

    void createSubrepoSuccess(const ServerRepo &repo, const QString& repo_id);
    void createSubrepoFailed(const ApiError& error);

private slots:
    void onGetDirentsSuccess(bool current_readonly, const QList<SeafDirent>& dirents, const QString& repo_id);
    void onFileUploadFinished(bool success);
    void onFileDownloadFinished(bool success);
    void onGetRepoFailed(const ApiError& error);
    void onGetRepoSuccess(const ServerRepo& repo, QString repo_id, QString path, QString file_id, QString local_path);

    void onCreateDirectorySuccess(const QString& repo_id);
    void onLockFileSuccess(const QString& repo_id);
    void onRenameDirentSuccess(const QString& repo_id);
    void onRemoveDirentSuccess(const QString& repo_id);
    void onRemoveDirentsSuccess(const QString& repo_id);
    void onCopyDirentsSuccess(const QString& dst_repo_id);
    void onCopyDirentsFailed(const ApiError& error);
    void onMoveDirentsSuccess(const QString& dst_repo_id);
    void onMoveDirentsFailed(const ApiError& error);

    void onCreateSubrepoSuccess(const QString& new_repoid);
    void onCreateSubrepoRefreshSuccess(const ServerRepo& new_repo);

    void onAccountChanged();
    void slotShareDirectSuccess(const QString& link);

public slots:
    // async copy operation
    void slotAsyncCopyMutipleItemsSuccess(const QString& task_id);
    void slotAsyncCopyMutipleItemsFailed(const ApiError& error);
    void asyncCopyOneItemApi();
    void slotAsyncCopyOneItemSuccess(const QString& task_id);
    void slotAsyncCopyOneItemFailed(const ApiError& error);
    void slotQueryAsyncCopyOperaProgress();
    void slotQueryAsyncCopyOperationProgressSuccess();
    void slotQueryAsyncCopyOperationProgressFailed(const ApiError& error);

    // async move operations
    void slotAsyncMoveMutipleItemsSuccess(const QString& task_id);
    void slotAsyncMoveMutipleItemsFailed(const ApiError& error);
    void asyncMoveOneItemApi();
    void slotAsyncMoveOneItemSuccess(const QString& task_id);
    void slotAsyncMoveOneItemFailed(const ApiError& error);
    void slotQueryAsyncMoveOperaProgress();
    void slotQueryAsyncMoveOperationProgressSuccess();
    void slotQueryAsyncMoveOperationProgressFailed(const ApiError& error);

private:
    void removeDirentsCache(const QString& repo_id,
                            const QString& path,
                            bool is_file);
    void setupTaskCleanup(FileNetworkTask *task);
    Account account_;

    QScopedPointer<CreateSubrepoRequest, QScopedPointerDeleteLater> create_subrepo_req_;
    QString create_subrepo_parent_repo_id_;
    QString create_subrepo_parent_path_;
    QScopedPointer<GetRepoRequest, QScopedPointerDeleteLater> get_repo_req_;

    QList<SeafileApiRequest*> reqs_;

    FileCache *filecache_;

    DirentsCache *dirents_cache_;
    QString old_repo_id_;

    // copy and move struct
    QMap<QString, int> src_dirents_;
    QString repo_id_;
    QString dir_path_;
    QString dst_repo_id_;
    QString dst_dir_path_;
    bool is_batch_operation_;
    QString task_id_;

    static QHash<QString, std::pair<qint64, QString> > passwords_cache_;
    QTimer* query_async_opera_progress_timer_;
    QScopedPointer<AsyncCopyAndMoveOneItemRequest, QScopedPointerDeleteLater> async_copy_one_item_req_;
    bool copy_move_in_progress_;
};


#endif // SEAFILE_CLIENT_FILE_BROWSER_DATA_MANAGER_H
