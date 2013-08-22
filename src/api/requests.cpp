#include <QtNetwork>
#include <jansson.h>
#include <QScopedPointer>

#include "account.h"

#include "requests.h"

namespace {

const char *kApiLoginUrl = "/api2/auth-token/";
const char *kListReposUrl = "/api2/repos/";

} // namespace


/**
 * LoginRequest
 */
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
    json_error_t error;
    json_t *root = parseJSON(reply, &error);
    if (!root) {
        qDebug("failed to parse json:%s\n", error.text);
        emit failed(0);
        return;
    }

    QScopedPointer<json_t, JsonPointerCustomDeleter> json(root);

    const char *token = json_string_value(json_object_get(json.data(), "token"));
    if (token == NULL) {
        qDebug("failed to parse json:%s\n", error.text);
        emit failed(0);
        return;
    }

    qDebug("login successful, token is %s\n", token);

    emit success(token);
}


/**
 * ListReposRequest
 */
ListReposRequest::ListReposRequest(const Account& account)
    : SeafileApiRequest (QUrl(account.serverUrl.toString() + kListReposUrl),
                         SeafileApiRequest::METHOD_GET, account.token)
{
}

void ListReposRequest::requestSuccess(QNetworkReply& reply)
{
    json_error_t error;
    json_t *root = parseJSON(reply, &error);
    if (!root) {
        qDebug("failed to parse json:%s\n", error.text);
        emit failed(0);
        return;
    }

    QScopedPointer<json_t, JsonPointerCustomDeleter> json(root);

    std::vector<ServerRepo> repos = ServerRepo::listFromJSON(json.data(), &error);
    emit success(repos);
}
