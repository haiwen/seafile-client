#include <QtNetwork>
#include <jansson.h>
#include <QScopedPointer>

#include "account.h"

#include "requests.h"
#include "server-repo.h"

namespace {

const char *kApiLoginUrl = "/api2/auth-token/";
const char *kListReposUrl = "/api2/repos/";
const char *kCreateRepoUrl = "/api2/repos/";

QMap<QString, QString> mapFromJSON(json_t *json, json_error_t */* error */)
{
    QMap<QString, QString> dict;
    void *member;
    const char *key;
    json_t *value;

    for (member = json_object_iter(json); member; member = json_object_iter_next(json, member)) {
        key = json_object_iter_key(member);
        value = json_object_iter_value(member);

        qDebug() << "key:"<<key << " value:" << json_string_value(value);
        dict[QString::fromUtf8(key)] = QString::fromUtf8(json_string_value(value));
    }
    return dict;
}

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
        qDebug("ListReposRequest:failed to parse json:%s\n", error.text);
        emit failed(0);
        return;
    }

    QScopedPointer<json_t, JsonPointerCustomDeleter> json(root);

    std::vector<ServerRepo> repos = ServerRepo::listFromJSON(json.data(), &error);
    emit success(repos);
}


/**
 * DownloadRepoRequest
 */
DownloadRepoRequest::DownloadRepoRequest(const Account& account, ServerRepo *repo)
    : SeafileApiRequest (QUrl(account.serverUrl.toString() + "/api2/repos/" + repo->id + "/download-info/"),
                         SeafileApiRequest::METHOD_GET, account.token),
      repo_(repo)
{
    connect(this, SIGNAL(failed(int)), this, SLOT(requestFailed(int)));
}

void DownloadRepoRequest::requestSuccess(QNetworkReply& reply)
{
    json_error_t error;
    json_t *root = parseJSON(reply, &error);
    if (!root) {
        qDebug("failed to parse json:%s\n", error.text);
        emit failed(0);
        return;
    }

    QScopedPointer<json_t, JsonPointerCustomDeleter> json(root);
    QMap<QString, QString> dict = mapFromJSON(json.data(), &error);
    emit success(dict, repo_);
}

void DownloadRepoRequest::requestFailed(int error)
{
    emit fail(error, repo_);
}


/**
 * CreateRepoRequest
 */
CreateRepoRequest::CreateRepoRequest(const Account& account, QString &name, QString &desc, QString &passwd)
    : SeafileApiRequest (QUrl(account.serverUrl.toString() + kCreateRepoUrl),
                         SeafileApiRequest::METHOD_POST, account.token)
{
    this->setParam(QString("name"), name);
    this->setParam(QString("desc"), desc);
    if (!passwd.isNull()) {
        qDebug("Encrypted repo");
        this->setParam(QString("passwd"), passwd);
    }
}

void CreateRepoRequest::requestSuccess(QNetworkReply& reply)
{
    json_error_t error;
    json_t *root = parseJSON(reply, &error);
    if (!root) {
        qDebug("failed to parse json:%s\n", error.text);
        emit failed(0);
        return;
    }

    QScopedPointer<json_t, JsonPointerCustomDeleter> json(root);
    QMap<QString, QString> dict = mapFromJSON(json.data(), &error);
    emit success(dict);
}
