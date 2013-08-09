#include <QtNetwork>
#include <jansson.h>

#include "login-request.h"

namespace {

const char *kApiLoginUrl = "/api2/auth-token/";

} // namespace

LoginRequest::LoginRequest(const QUrl& serverAddr,
                           const QString& username,
                           const QString& password)

    : SeafileApiRequest (QUrl(serverAddr.toString() + kApiLoginUrl),
                         SeafileApiRequest::METHOD_POST)
{
    setParam("username", username);
    setParam("password", password);
}

void LoginRequest::requestSuccess(QNetworkReply& reply)
{
    QByteArray raw = reply.readAll();
    json_error_t error;
    json_t *root = json_loads(raw.data(), 0, &error);
    if (!root) {
        qDebug("failed to parse json:%s\n", error.text);
        emit failed(0);
        return;
    }

    const char *token = json_string_value(json_object_get(root, "token"));
    if (token == NULL) {
        qDebug("failed to parse json:%s\n", error.text);
        emit failed(0);
        return;
    }

    qDebug("login successful, token is %s\n", token);

    emit success(token);
}
