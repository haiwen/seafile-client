#ifndef SEAFILE_CLIENT_FILE_BROWSER_REQUESTS_H
#define SEAFILE_CLIENT_FILE_BROWSER_REQUESTS_H

#include <QList>

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
                      const QString& path,
                      const QString& cached_dir_id=QString());

    QString repoId() const { return repo_id_; }
    QString path() const { return path_; }

signals:
    void success(const QString &dir_id,
                 const QList<SeafDirent> &dirents);

protected slots:
    void requestSuccess(QNetworkReply& reply);

private:
    Q_DISABLE_COPY(GetDirentsRequest)

    QString repo_id_;
    QString path_;
    QString cached_dir_id_;
};

class GetDirentSharedLinkRequest : public SeafileApiRequest {
    Q_OBJECT
public:
    GetDirentSharedLinkRequest(const Account &account, const QString &repo_id,
                               const QString &path);

signals:
    void success(const QString& url);

protected slots:
    void requestSuccess(QNetworkReply& reply);

private:
    Q_DISABLE_COPY(GetDirentSharedLinkRequest)
};

class CreateDirentRequest : public SeafileApiRequest {
    Q_OBJECT
public:
    CreateDirentRequest(const Account &account, const QString &repo_id,
                        const QString &path);

signals:
    void success(const QString& url);

protected slots:
    void requestSuccess(QNetworkReply& reply);

private:
    Q_DISABLE_COPY(CreateDirentRequest)
};

class RenameDirentRequest : public SeafileApiRequest {
    Q_OBJECT
public:
    RenameDirentRequest(const Account &account, const QString &repo_id,
                        const QString &path, const QString &new_path);

signals:
    void success(const QString& url);

protected slots:
    void requestSuccess(QNetworkReply& reply);

private:
    Q_DISABLE_COPY(RenameDirentRequest)
};

class RemoveDirentRequest : public SeafileApiRequest {
    Q_OBJECT
public:
    RemoveDirentRequest(const Account &account, const QString &repo_id,
                        const QString &path);

signals:
    void success(const QString& url);

protected slots:
    void requestSuccess(QNetworkReply& reply);

private:
    Q_DISABLE_COPY(RemoveDirentRequest)
};

class GetFileDownloadRequest : public SeafileApiRequest {
    Q_OBJECT
public:
    GetFileDownloadRequest(const Account &account, const QString &repo_id,
                           const QString &path);

signals:
    void success(const QString& url, const QString& oid);

protected slots:
    void requestSuccess(QNetworkReply& reply);

private:
    Q_DISABLE_COPY(GetFileDownloadRequest)
};

class GetFileSharedLinkRequest : public SeafileApiRequest {
    Q_OBJECT
public:
    GetFileSharedLinkRequest(const Account &account, const QString &repo_id,
                             const QString &path);

signals:
    void success(const QString& url);

protected slots:
    void requestSuccess(QNetworkReply& reply);

private:
    Q_DISABLE_COPY(GetFileSharedLinkRequest)
};

class GetFileUploadRequest : public SeafileApiRequest {
    Q_OBJECT
public:
    GetFileUploadRequest(const Account &account, const QString &repo_id,
                         const QString &path);

signals:
    void success(const QString& url);

protected slots:
    void requestSuccess(QNetworkReply& reply);

private:
    Q_DISABLE_COPY(GetFileUploadRequest)
};

class GetFileUpdateRequest : public SeafileApiRequest {
    Q_OBJECT
public:
    GetFileUpdateRequest(const Account &account, const QString &repo_id,
                         const QString &path);

signals:
    void success(const QString& url);

protected slots:
    void requestSuccess(QNetworkReply& reply);

private:
    Q_DISABLE_COPY(GetFileUpdateRequest)
};

class RenameFileRequest : public SeafileApiRequest {
    Q_OBJECT
public:
    RenameFileRequest(const Account &account, const QString &repo_id,
                      const QString &path, const QString &new_name);

signals:
    void success();

protected slots:
    void requestSuccess(QNetworkReply& reply);

private:
    Q_DISABLE_COPY(RenameFileRequest)
};

class MoveFileRequest : public SeafileApiRequest {
    Q_OBJECT
public:
    MoveFileRequest(const Account &account,
                    const QString &repo_id,
                    const QString &path,
                    const QString &new_repo_id,
                    const QString &new_path);

signals:
    void success();

protected slots:
    void requestSuccess(QNetworkReply& reply);

private:
    Q_DISABLE_COPY(MoveFileRequest)
};

class RemoveFileRequest : public SeafileApiRequest {
    Q_OBJECT
public:
    RemoveFileRequest(const Account &account, const QString &repo_id,
                      const QString &path);

signals:
    void success();

protected slots:
    void requestSuccess(QNetworkReply& reply);

private:
    Q_DISABLE_COPY(RemoveFileRequest)
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

#endif  // SEAFILE_CLIENT_FILE_BROWSER_REQUESTS_H
