#ifndef SEAFILE_CLIENT_API_REQUESTS_H
#define SEAFILE_CLIENT_API_REQUESTS_H

#include <vector>

#include "api-request.h"
#include "server-repo.h"

class QNetworkReply;
class Account;

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


#endif // SEAFILE_CLIENT_API_REQUESTS_H
