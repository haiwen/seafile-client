#ifndef SEAFILE_CLIENT_API_REQUESTS_H
#define SEAFILE_CLIENT_API_REQUESTS_H

#include <vector>

#include "api-request.h"
#include "server-repo.h"

class QNetworkReply;
struct Account;
class ServerRepo;

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


struct RepoDownloadInfo {
    QString relay_id;
    QString relay_addr;
    QString relay_port;
    QString email;
    QString token;
    QString repo_id;
    QString repo_name;
    QString encrypted;
    QString magic;
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
    void success(const QMap<QString, QString> &dict);

private:
    Q_DISABLE_COPY(CreateRepoRequest)

};


#endif // SEAFILE_CLIENT_API_REQUESTS_H
