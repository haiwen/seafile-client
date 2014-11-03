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
                      const QString& path);

    QString repoId() const { return repo_id_; }
    QString path() const { return path_; }

signals:
    void success(const QList<SeafDirent> &dirents);

protected slots:
    void requestSuccess(QNetworkReply& reply);

private:
    Q_DISABLE_COPY(GetDirentsRequest)

    QString repo_id_;
    QString path_;
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
    void success();

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
    void success();

protected slots:
    void requestSuccess(QNetworkReply& reply);

private:
    Q_DISABLE_COPY(RemoveDirentRequest)
};

class GetSharedLinkRequest : public SeafileApiRequest {
    Q_OBJECT
public:
    GetSharedLinkRequest(const Account &account, const QString &repo_id,
                             const QString &path, bool is_file);

signals:
    void success(const QString& url);

protected slots:
    void requestSuccess(QNetworkReply& reply);

private:
    Q_DISABLE_COPY(GetSharedLinkRequest)
};

class GetFileUploadLinkRequest : public SeafileApiRequest {
    Q_OBJECT
public:
    GetFileUploadLinkRequest(const Account &account,
                             const QString &repo_id);

signals:
    void success(const QString& url);

protected slots:
    void requestSuccess(QNetworkReply& reply);

private:
    Q_DISABLE_COPY(GetFileUploadLinkRequest)
};

class GetFileUpdateLinkRequest : public SeafileApiRequest {
    Q_OBJECT
public:
    GetFileUpdateLinkRequest(const Account &account,
                             const QString &repo_id);

signals:
    void success(const QString& url);

protected slots:
    void requestSuccess(QNetworkReply& reply);

private:
    Q_DISABLE_COPY(GetFileUpdateLinkRequest)
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
