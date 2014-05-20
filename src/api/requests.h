#ifndef SEAFILE_CLIENT_API_REQUESTS_H
#define SEAFILE_CLIENT_API_REQUESTS_H

#include <vector>
#include <QMap>

#include "api-request.h"
#include "server-repo.h"

class QNetworkReply;

class ServerRepo;
struct Account;
class StarredFile;

class LoginRequest : public SeafileApiRequest {
    Q_OBJECT

public:
    LoginRequest(const QUrl& serverAddr,
                 const QString& username,
                 const QString& password,
                 const QString& computer_name);

protected slots:
    void requestSuccess(QNetworkReply& reply);

signals:
    void success(const QString& token);

private:
    Q_DISABLE_COPY(LoginRequest)
};


class ListReposRequest : public SeafileApiRequest {
    Q_OBJECT

public:
    explicit ListReposRequest(const Account& account);

protected slots:
    void requestSuccess(QNetworkReply& reply);

signals:
    void success(const std::vector<ServerRepo>& repos);

private:
    Q_DISABLE_COPY(ListReposRequest)
};


class RepoDownloadInfo {
public:
    int repo_version;
    QString relay_id;
    QString relay_addr;
    QString relay_port;
    QString email;
    QString token;
    QString repo_id;
    QString repo_name;
    bool encrypted;
    int enc_version;
    QString magic;
    QString random_key;

    static RepoDownloadInfo fromDict(QMap<QString, QVariant>& dict);
};

class DownloadRepoRequest : public SeafileApiRequest {
    Q_OBJECT

public:
    explicit DownloadRepoRequest(const Account& account, const QString& repo_id);

protected slots:
    void requestSuccess(QNetworkReply& reply);

signals:
    void success(const RepoDownloadInfo& info);

private:
    Q_DISABLE_COPY(DownloadRepoRequest)
};

class CreateRepoRequest : public SeafileApiRequest {
    Q_OBJECT

public:
    explicit CreateRepoRequest(const Account& account, QString &name, QString &desc, QString &passwd);

protected slots:
    void requestSuccess(QNetworkReply& reply);

signals:
    void success(const RepoDownloadInfo& info);

private:
    Q_DISABLE_COPY(CreateRepoRequest)

};

class GetUnseenSeahubMessagesRequest : public SeafileApiRequest {
    Q_OBJECT

public:
    explicit GetUnseenSeahubMessagesRequest(const Account& account);

protected slots:
    void requestSuccess(QNetworkReply& reply);

signals:
    void success(int count);

private:
    Q_DISABLE_COPY(GetUnseenSeahubMessagesRequest)

};

class GetDefaultRepoRequest : public SeafileApiRequest {
    Q_OBJECT
public:
    GetDefaultRepoRequest(const Account& account);

signals:
    void success(bool exists, const QString& repo_id);

protected slots:
    void requestSuccess(QNetworkReply& reply);

private:
    Q_DISABLE_COPY(GetDefaultRepoRequest);
};

class CreateDefaultRepoRequest : public SeafileApiRequest {
    Q_OBJECT
public:
    CreateDefaultRepoRequest(const Account& account);

signals:
    void success(const QString& repo_id);

protected slots:
    void requestSuccess(QNetworkReply& reply);

private:
    Q_DISABLE_COPY(CreateDefaultRepoRequest);
};

class GetLatestVersionRequest : public SeafileApiRequest {
    Q_OBJECT
public:
    GetLatestVersionRequest(const QString& client_id, const QString& client_version);

signals:
    void success(const QString& latest_version);

protected slots:
    void requestSuccess(QNetworkReply& reply);

private:
    Q_DISABLE_COPY(GetLatestVersionRequest);
};

class GetStarredFilesRequest : public SeafileApiRequest {
    Q_OBJECT
public:
    GetStarredFilesRequest(const Account& account);

signals:
    void success(const std::vector<StarredFile>& starred_files);

protected slots:
    void requestSuccess(QNetworkReply& reply);

private:
    Q_DISABLE_COPY(GetStarredFilesRequest);
};

#endif // SEAFILE_CLIENT_API_REQUESTS_H
