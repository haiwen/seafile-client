#ifndef SEAFILE_CLIENT_FILE_BROWSER_REQUESTS_H
#define SEAFILE_CLIENT_FILE_BROWSER_REQUESTS_H

#include <QList>
#include <QStringList>
#include <QTimer>

#include "api/api-request.h"
#include "seaf-dirent.h"

class SeafDirent;
class Account;
class QDir;

class GetDirentsRequest : public SeafileApiRequest {
    Q_OBJECT
public:
    GetDirentsRequest(const Account& account,
                      const QString& repo_id,
                      const QString& path);

    const QString& repoId() const { return repo_id_; }
    const QString& path() const { return path_; }

signals:
    void success(bool current_readonly, const QList<SeafDirent> &dirents, const QString& repo_id);
    void failed(const ApiError& error, const QString& repo_id);

protected slots:
    void requestSuccess(QNetworkReply& reply);

private:
    Q_DISABLE_COPY(GetDirentsRequest)

    const QString repo_id_;
    const QString path_;
    bool readonly_;
};

class GetFileDownloadLinkRequest : public SeafileApiRequest {
    Q_OBJECT
public:
    GetFileDownloadLinkRequest(const Account &account,
                               const QString &repo_id,
                               const QString &path);

    QString fileId() const { return file_id_; }
signals:
    void success(const QString& url);

protected slots:
    void requestSuccess(QNetworkReply& reply);

private:
    Q_DISABLE_COPY(GetFileDownloadLinkRequest)

    QString file_id_;
};

// TODO:
// intergrate file creation into this class
class CreateDirectoryRequest : public SeafileApiRequest {
    Q_OBJECT
public:
    CreateDirectoryRequest(const Account &account, const QString &repo_id,
                           const QString &path, bool create_parents = false);
    const QString &repoId() { return repo_id_; }
    const QString &path() { return path_; }

signals:
    void success(const QString& repo_id);

protected slots:
    void requestSuccess(QNetworkReply& reply);

private:
    Q_DISABLE_COPY(CreateDirectoryRequest)
    const QString repo_id_;
    const QString path_;
    bool create_parents_;
};

class RenameDirentRequest : public SeafileApiRequest {
    Q_OBJECT
public:
    RenameDirentRequest(const Account &account, const QString &repo_id,
                        const QString &path, const QString &new_path,
                        bool is_file = true);

    const bool& isFile() const { return is_file_; }
    const QString& repoId() const { return repo_id_; }
    const QString& path() const { return path_; }
    const QString& newName() const { return new_name_; }

signals:
    void success(const QString& repo_id);

protected slots:
    void requestSuccess(QNetworkReply& reply);

private:
    Q_DISABLE_COPY(RenameDirentRequest)

    const bool is_file_;
    const QString repo_id_;
    const QString path_;
    const QString new_name_;
};

class RemoveDirentRequest : public SeafileApiRequest {
    Q_OBJECT
public:
    RemoveDirentRequest(const Account &account, const QString &repo_id,
                        const QString &path, bool is_file = true);

    const bool& isFile() const { return is_file_; }
    const QString& repoId() const { return repo_id_; }
    const QString& path() const { return path_; }

signals:
    void success(const QString& repo_id);

protected slots:
    void requestSuccess(QNetworkReply& reply);

private:
    Q_DISABLE_COPY(RemoveDirentRequest)

    const bool is_file_;
    const QString repo_id_;
    const QString path_;
};

class RemoveDirentsRequest : public SeafileApiRequest {
    Q_OBJECT
public:
    RemoveDirentsRequest(const Account &account,
                         const QString &repo_id,
                         const QString &parent_path,
                         const QStringList& filenames);

    const QString& repoId() const { return repo_id_; }
    const QString& parentPath() const { return parent_path_; }
    const QStringList& filenames() const { return filenames_; }

signals:
    void success(const QString& repo_id);

protected slots:
    void requestSuccess(QNetworkReply& reply);

private:
    Q_DISABLE_COPY(RemoveDirentsRequest)

    const QString repo_id_;
    const QString parent_path_;
    const QStringList filenames_;
};


class GetSharedLinkRequest : public SeafileApiRequest {
    Q_OBJECT
public:
    GetSharedLinkRequest(const Account &account, const QString &repo_id,
                             const QString &path, bool is_file);

signals:
    void success(const QString& url, const QString& repo_id);

protected slots:
    void requestSuccess(QNetworkReply& reply);

private:
    Q_DISABLE_COPY(GetSharedLinkRequest)
    const QString repo_id_;
};

class GetFileUploadLinkRequest : public SeafileApiRequest {
    Q_OBJECT
public:
    GetFileUploadLinkRequest(const Account &account,
                             const QString &repo_id,
                             const QString &path,
                             bool use_upload = true);

signals:
    void success(const QString& url);

protected slots:
    void requestSuccess(QNetworkReply& reply);

private:
    Q_DISABLE_COPY(GetFileUploadLinkRequest)
};

// Query asynchronous operation progress
class QueryAsyncOperationProgress : public SeafileApiRequest {
Q_OBJECT
public:
    QueryAsyncOperationProgress(const Account &account,
                                const QString &task_id);
signals:
    void success();

private slots:
    void requestSuccess(QNetworkReply& reply);

};

// Async copy and move a single item
class AsyncCopyAndMoveOneItemRequest : public SeafileApiRequest {
Q_OBJECT
public:
    AsyncCopyAndMoveOneItemRequest(const Account &account,
                                   const QString &src_repo_id,
                                   const QString &src_parent_dir,
                                   const QString &src_dirent_name,
                                   const QString &dst_repo_id,
                                   const QString &dst_parent_dir,
                                   const QString &operation,
                                   const QString &dirent_type);

signals:
    void success(const QString& dst_repo_id);
    void sigRequestSuccess(QNetworkReply& reply);

protected slots:
    void requestSuccess(QNetworkReply& reply);

private:
    const Account& account_;
    const QString repo_id_;
    const QString src_dir_path_;
    const QString src_dirent_name_;
    const QString dst_repo_id_;
    const QString dst_repo_path_;
    const QString operation_;
    const QString dirent_type_;

    Q_DISABLE_COPY(AsyncCopyAndMoveOneItemRequest)
};

// Single File only
class MoveFileRequest : public SeafileApiRequest {
    Q_OBJECT
public:
    MoveFileRequest(const Account &account,
                    const QString &repo_id,
                    const QString &path,
                    const QString &dst_repo_id,
                    const QString &dst_dir_path);

signals:
    void success();

protected slots:
    void requestSuccess(QNetworkReply& reply);

private:
    Q_DISABLE_COPY(MoveFileRequest)
};

class CopyMultipleFilesRequest : public SeafileApiRequest {
    Q_OBJECT
public:
    CopyMultipleFilesRequest(const Account &account,
                             const QString &repo_id,
                             const QString &src_dir_path,
                             const QStringList &src_file_names,
                             const QString &dst_repo_id,
                             const QString &dst_dir_path);
    const QString& repoId() { return repo_id_; }
    const QString& srcPath() { return src_dir_path_; }
    const QStringList& srcFileNames() { return src_file_names_; }

signals:
    void success(const QString& dst_repo_id);

protected slots:
    void requestSuccess(QNetworkReply& reply);

private:
    Q_DISABLE_COPY(CopyMultipleFilesRequest)
    const QString repo_id_;
    const QString src_dir_path_;
    const QStringList src_file_names_;
    const QString dst_repo_id_;
};

class MoveMultipleFilesRequest : public SeafileApiRequest {
    Q_OBJECT
public:
    MoveMultipleFilesRequest(const Account &account,
                             const QString &repo_id,
                             const QString &src_dir_path,
                             const QStringList &src_file_names,
                             const QString &dst_repo_id,
                             const QString &dst_dir_path);
    const QString& srcRepoId() { return repo_id_; }
    const QString& srcPath() { return src_dir_path_; }
    const QStringList& srcFileNames() { return src_file_names_; }

signals:
    void success(const QString& dst_repo_id);

protected slots:
    void requestSuccess(QNetworkReply& reply);

private:
    Q_DISABLE_COPY(MoveMultipleFilesRequest)
    const QString repo_id_;
    const QString src_dir_path_;
    const QStringList src_file_names_;
    const QString dst_repo_id_;
};

// Batch copy items asynchronously
class AsyncCopyMultipleItemsRequest : public SeafileApiRequest {
Q_OBJECT
public:

    AsyncCopyMultipleItemsRequest (const Account &account, const QString &repo_id,
                                   const QString &src_dir_path,
                                   const QMap<QString, int>&src_dirents,
                                   const QString &dst_repo_id,
                                   const QString &dst_dir_path);
signals:
    void success(const QString& dst_repo_id);
    void sigAsyncCopyMultipleItemsFailed(const ApiError&);

protected slots:
    void requestSuccess(QNetworkReply& reply);
    void slotInvokeSyncBatchCopyV2(const ApiError& api_error);

public slots:
    void queryAsyncCopyMoveOneItemProgressSuccess();
    void slotAsyncCopyOneItemSuccess(QNetworkReply& reply);

private slots:
    void useAsyncCopyMoveOneItemApi();
    void slotQueryAsyncOperationProgress();
    void slotQueryAsyncBatchOperationProgress();
    void slotQueryAsyncBatchOperationProgressResponseSuccess();

private:
    Q_DISABLE_COPY(AsyncCopyMultipleItemsRequest)
    const Account& account_;
    const QString repo_id_;
    const QString src_dir_path_;
    QMap<QString, int> src_dirents_;
    const QString dst_repo_id_;
    const QString dst_repo_path_;
    QString task_id_;
    QTimer* query_async_batch_opera_progress_timer_;
    QTimer* query_async_opera_progress_timer_;
    QScopedPointer<AsyncCopyAndMoveOneItemRequest, QScopedPointerDeleteLater> async_copy_one_item_req_;
    QScopedPointer<QueryAsyncOperationProgress, QScopedPointerDeleteLater> query_async_opera_progress_req_;
};

// Batch move items asynchronously
class AsyncMoveMultipleItemsRequest : public SeafileApiRequest {
Q_OBJECT
public:
    AsyncMoveMultipleItemsRequest(const Account &account,
                                  const QString &repo_id,
                                  const QString &src_dir_path,
                                  const QMap<QString, int> &src_dirents,
                                  const QString &dst_repo_id,
                                  const QString &dst_dir_path);
    const QString& getSrcRepoId() { return repo_id_; }
    const QString& getSrcPath() { return src_dir_path_; }

signals:
    void success(const QString& dst_repo_id);
    void sigAsyncMoveMultipleItemsFailed(const ApiError&);

protected slots:
    void requestSuccess(QNetworkReply& reply);
    void slotInvokeSyncBatchMoveV2(const ApiError& api_error);

public slots:
    void queryAsyncCopyMoveOneItemProgressSuccess();
    void slotAsyncMoveOneItemSuccess(QNetworkReply& reply);

private slots:
    void useAsyncCopyMoveOneItemApi();
    void slotQueryAsyncOperationProgress();
    void slotQueryAsyncBatchOperationProgress();
    void slotQueryAsyncBatchOperationProgressResponseSuccess();

private:
    Q_DISABLE_COPY(AsyncMoveMultipleItemsRequest)
    const Account& account_;
    const QString repo_id_;
    const QString src_dir_path_;
    QMap<QString, int> src_dirents_;
    const QString dst_repo_id_;
    const QString dst_repo_path_;
    QString task_id_;
    QTimer* query_async_opera_progress_timer_;
    QTimer* query_async_batch_opera_progress_timer_;
    QScopedPointer<AsyncCopyAndMoveOneItemRequest, QScopedPointerDeleteLater> async_move_one_item_req_;
    QScopedPointer<QueryAsyncOperationProgress, QScopedPointerDeleteLater> query_async_opera_progress_req_;
};

class StarFileRequest : public SeafileApiRequest {
    Q_OBJECT
public:
    StarFileRequest(const Account &account, const QString &repo_id,
                    const QString &path);

signals:
    void success();

protected slots:
    void requestSuccess(QNetworkReply& reply);

private:
    Q_DISABLE_COPY(StarFileRequest)
};

class UnstarFileRequest : public SeafileApiRequest {
    Q_OBJECT
public:
    UnstarFileRequest(const Account &account, const QString &repo_id,
                      const QString &path);

signals:
    void success();

protected slots:
    void requestSuccess(QNetworkReply& reply);

private:
    Q_DISABLE_COPY(UnstarFileRequest)
};

class LockFileRequest : public SeafileApiRequest {
    Q_OBJECT
public:
    LockFileRequest(const Account& account,
                    const QString& repo_id,
                    const QString& path,
                    bool lock);

    bool lock() const { return lock_; }
    const QString & repoId() const { return repo_id_; }
    const QString & path() const { return path_; }

signals:
    void success(const QString& repo_id);

protected slots:
    void requestSuccess(QNetworkReply& reply);

private:
    Q_DISABLE_COPY(LockFileRequest);
    const bool lock_;
    const QString repo_id_;
    const QString path_;
};

class GetFileUploadedBytesRequest : public SeafileApiRequest {
    Q_OBJECT
public:
    GetFileUploadedBytesRequest(const Account& account,
                                const QString& repo_id,
                                const QString& parent_dir,
                                const QString& file_name);

    const QString & repoId() const { return repo_id_; }
    const QString & parentDir() const { return parent_dir_; }
    const QString & fileName() const { return file_name_; }

signals:
    void success(bool support_chunked_uploading, quint64 uploaded_bytes);

protected slots:
    void requestSuccess(QNetworkReply& reply);

private:
    Q_DISABLE_COPY(GetFileUploadedBytesRequest);
    const QString repo_id_;
    const QString parent_dir_;
    const QString file_name_;
};

struct ServerIndexProgress {
    qint64 total;
    qint64 indexed;
    qint64 status;
};

class GetIndexProgressRequest : public SeafileApiRequest {
    Q_OBJECT
public:
    GetIndexProgressRequest(const QUrl &url, const QString &task_id);
signals:
    void success(const ServerIndexProgress& result);

protected slots:
    void requestSuccess(QNetworkReply& reply);

private:
    Q_DISABLE_COPY(GetIndexProgressRequest);
};

class GetSmartLinkRequest : public SeafileApiRequest
{
    Q_OBJECT
public:
    GetSmartLinkRequest(const Account& account,
                        const QString& repo_id,
                        const QString& path,
                        bool is_dir);

signals:
    void success(const QString &smart_link, const QString &protocol_link);

protected slots:
    void requestSuccess(QNetworkReply& reply);

private:
    Q_DISABLE_COPY(GetSmartLinkRequest);
    QString repo_id_;
    QString path_;
    QString protocol_link_;
    bool is_dir_;
};

class GetFileLockInfoRequest : public SeafileApiRequest
{
    Q_OBJECT
public:
    GetFileLockInfoRequest(const Account& account,
                           const QString& repo_id,
                           const QString& path);

    virtual void send() Q_DECL_OVERRIDE;

    const QString& path() const { return path_; }

signals:
    void success(bool found, const QString& lock_owner);

protected slots:
    void requestSuccess(QNetworkReply& reply);
    void onGetDirentsSuccess(bool current_readonly, const QList<SeafDirent> &dirents);

private:
    Q_DISABLE_COPY(GetFileLockInfoRequest);

    const QString path_;
    QScopedPointer<GetDirentsRequest, QScopedPointerDeleteLater> dirents_req_;
};

class GetUploadLinkRequest : public SeafileApiRequest
{
Q_OBJECT
public:
    GetUploadLinkRequest(const Account& account,
                         const QString& repo_id,
                         const QString& path);
    const QString& path() const { return path_; }
signals:
    void success(const QString& upload_link);

protected slots:
    void requestSuccess(QNetworkReply& reply);

private:
    Q_DISABLE_COPY(GetUploadLinkRequest);
    QString path_;
};

#endif  // SEAFILE_CLIENT_FILE_BROWSER_REQUESTS_H
