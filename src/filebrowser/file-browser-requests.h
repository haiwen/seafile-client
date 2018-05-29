#ifndef SEAFILE_CLIENT_FILE_BROWSER_REQUESTS_H
#define SEAFILE_CLIENT_FILE_BROWSER_REQUESTS_H

#include <QList>
#include <QStringList>

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

#endif  // SEAFILE_CLIENT_FILE_BROWSER_REQUESTS_H
