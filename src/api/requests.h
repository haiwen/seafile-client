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

class DownloadRepoRequest : public SeafileApiRequest {
    Q_OBJECT

public:
    explicit DownloadRepoRequest(const Account& account, ServerRepo *repo);

protected slots:
    void requestSuccess(QNetworkReply& reply);
    void requestFailed(int error);

signals:
    void success(const QMap<QString, QString> &map, ServerRepo *repo);
    void fail(int code, ServerRepo *repo);

private:
    Q_DISABLE_COPY(DownloadRepoRequest)

    ServerRepo *repo_;
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
