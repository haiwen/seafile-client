#ifndef SEAFILE_LOGIN_REQUEST_H
#define SEAFILE_LOGIN_REQUEST_H

#include "api-request.h"

class QNetworkReply;

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

#endif // SEAFILE_LOGIN_REQUEST_H
