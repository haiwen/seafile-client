#ifndef SEAFILE_CLIENT_API_REQUESTS_H
#define SEAFILE_CLIENT_API_REQUESTS_H

#include <vector>
#include <QMap>

#include "api-request.h"
#include "server-repo.h"

class QNetworkReply;

class ServerRepo;
struct Account;

class LoginRequest : public SeafileApiRequest {
    Q_OBJECT

public:
    LoginRequest(const QUrl& serverAddr,
                 const QString& username,
                 const QString& password);

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
    explicit DownloadRepoRequest(const Account& account, const ServerRepo& repo);

protected slots:
    void requestSuccess(QNetworkReply& reply);

signals:
    void success(const RepoDownloadInfo& info);

private:
    Q_DISABLE_COPY(DownloadRepoRequest)

    ServerRepo repo_;
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

class GetSeahubMessagesRequest : public SeafileApiRequest {
    Q_OBJECT

public:
    explicit GetSeahubMessagesRequest(const Account& account);

protected slots:
    void requestSuccess(QNetworkReply& reply);

signals:
    void success(int group_messages, int personal_messages);

private:
    Q_DISABLE_COPY(GetSeahubMessagesRequest)

};

#endif // SEAFILE_CLIENT_API_REQUESTS_H
