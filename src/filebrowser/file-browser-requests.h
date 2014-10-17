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

#endif  // SEAFILE_CLIENT_FILE_BROWSER_REQUESTS_H
