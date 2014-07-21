#include <jansson.h>

#include <QtNetwork>
#include <QScopedPointer>

#include "account.h"

#include "seafile-applet.h"
#include "rpc/rpc-client.h"
#include "utils/utils.h"
#include "api-error.h"
#include "server-repo.h"
#include "starred-file.h"
#include "event.h"
#include "commit-details.h"

#include "requests.h"

namespace {

const char *kApiLoginUrl = "/api2/auth-token/";
const char *kListReposUrl = "/api2/repos/";
const char *kCreateRepoUrl = "/api2/repos/";
const char *kUnseenMessagesUrl = "/api2/unseen_messages/";
const char *kDefaultRepoUrl = "/api2/default-repo/";
const char *kStarredFilesUrl = "/api2/starredfiles/";
const char *kGetEventsUrl = "/api2/events/";
const char *kCommitDetailsUrl = "/api2/repo_history_changes/";
const char *kAvatarUrl = "/api2/avatars/user/";
const char *kSetRepoPasswordUrl = "/api2/repos/";

const char *kLatestVersionUrl = "http://seafile.com/api/client-versions/";

#if defined(Q_OS_WIN)
const char *kOsName = "windows";
#elif defined(Q_OS_LINUX)
const char *kOsName = "linux";
#else
const char *kOsName = "mac";
#endif

} // namespace


/**
 * LoginRequest
 */
LoginRequest::LoginRequest(const QUrl& serverAddr,
                           const QString& username,
                           const QString& password,
                           const QString& computer_name)

    : SeafileApiRequest (QUrl(serverAddr.toString() + kApiLoginUrl),
                         SeafileApiRequest::METHOD_POST)
{
    setParam("username", username);
    setParam("password", password);

    QString client_version = STRINGIZE(SEAFILE_CLIENT_VERSION);
    QString device_id = seafApplet->rpcClient()->getCcnetPeerId();

    setParam("platform", kOsName);
    setParam("device_id", device_id);
    setParam("device_name", computer_name);
    setParam("client_version", client_version);
    setParam("platform_version", "");
}

void LoginRequest::requestSuccess(QNetworkReply& reply)
{
    json_error_t error;
    json_t *root = parseJSON(reply, &error);
    if (!root) {
        qDebug("failed to parse json:%s\n", error.text);
        emit failed(ApiError::fromJsonError());
        return;
    }

    QScopedPointer<json_t, JsonPointerCustomDeleter> json(root);

    const char *token = json_string_value(json_object_get(json.data(), "token"));
    if (token == NULL) {
        qDebug("failed to parse json:%s\n", error.text);
        emit failed(ApiError::fromJsonError());
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
        emit failed(ApiError::fromJsonError());
        return;
    }

    QScopedPointer<json_t, JsonPointerCustomDeleter> json(root);

    std::vector<ServerRepo> repos = ServerRepo::listFromJSON(json.data(), &error);
    emit success(repos);
}


/**
 * DownloadRepoRequest
 */
DownloadRepoRequest::DownloadRepoRequest(const Account& account, const QString& repo_id)
    : SeafileApiRequest(QUrl(account.serverUrl.toString() + "/api2/repos/" + repo_id + "/download-info/"),
                        SeafileApiRequest::METHOD_GET, account.token)
{
}

RepoDownloadInfo RepoDownloadInfo::fromDict(QMap<QString, QVariant>& dict)
{
    RepoDownloadInfo info;
    info.repo_version = dict["repo_version"].toInt();
    info.relay_id = dict["relay_id"].toString();
    info.relay_addr = dict["relay_addr"].toString();
    info.relay_port = dict["relay_port"].toString();
    info.email = dict["email"].toString();
    info.token = dict["token"].toString();
    info.repo_id = dict["repo_id"].toString();
    info.repo_name = dict["repo_name"].toString();
    info.encrypted = dict["encrypted"].toInt();
    info.magic = dict["magic"].toString();
    info.random_key = dict["random_key"].toString();
    info.enc_version = dict.value("enc_version", 1).toInt();

    return info;
}

void DownloadRepoRequest::requestSuccess(QNetworkReply& reply)
{
    json_error_t error;
    json_t *root = parseJSON(reply, &error);
    if (!root) {
        qDebug("failed to parse json:%s\n", error.text);
        emit failed(ApiError::fromJsonError());
        return;
    }

    QScopedPointer<json_t, JsonPointerCustomDeleter> json(root);
    QMap<QString, QVariant> dict = mapFromJSON(json.data(), &error);

    RepoDownloadInfo info = RepoDownloadInfo::fromDict(dict);

    info.relay_addr = url().host();

    emit success(info);
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
        emit failed(ApiError::fromJsonError());
        return;
    }

    QScopedPointer<json_t, JsonPointerCustomDeleter> json(root);
    QMap<QString, QVariant> dict = mapFromJSON(json.data(), &error);
    RepoDownloadInfo info = RepoDownloadInfo::fromDict(dict);

    info.relay_addr = url().host();
    emit success(info);
}

/**
 * GetUnseenSeahubNotificationsRequest
 */
GetUnseenSeahubNotificationsRequest::GetUnseenSeahubNotificationsRequest(const Account& account)
    : SeafileApiRequest (QUrl(account.serverUrl.toString() + kUnseenMessagesUrl),
                         SeafileApiRequest::METHOD_GET, account.token)
{
}

void GetUnseenSeahubNotificationsRequest::requestSuccess(QNetworkReply& reply)
{
    json_error_t error;
    json_t *root = parseJSON(reply, &error);
    if (!root) {
        qDebug("GetUnseenSeahubNotificationsRequest: failed to parse json:%s\n", error.text);
        emit failed(ApiError::fromJsonError());
        return;
    }

    QScopedPointer<json_t, JsonPointerCustomDeleter> json(root);

    QMap<QString, QVariant> ret = mapFromJSON(root, &error);

    if (!ret.contains("count")) {
        emit failed(ApiError::fromJsonError());
        return;
    }

    int count = ret.value("count").toInt();
    emit success(count);
}

GetDefaultRepoRequest::GetDefaultRepoRequest(const Account& account)
    : SeafileApiRequest (QUrl(account.serverUrl.toString() + kDefaultRepoUrl),
                         SeafileApiRequest::METHOD_GET, account.token)
{
}

void GetDefaultRepoRequest::requestSuccess(QNetworkReply& reply)
{
    json_error_t error;
    json_t *root = parseJSON(reply, &error);
    if (!root) {
        qDebug("CreateDefaultRepoRequest: failed to parse json:%s\n", error.text);
        emit failed(ApiError::fromJsonError());
        return;
    }

    QScopedPointer<json_t, JsonPointerCustomDeleter> json(root);

    QMap<QString, QVariant> dict = mapFromJSON(json.data(), &error);

    if (!dict.contains("exists")) {
        emit failed(ApiError::fromJsonError());
        return;
    }

    bool exists = dict.value("exists").toBool();
    if (!exists) {
        emit success(false, "");
        return;
    }

    if (!dict.contains("repo_id")) {
        emit failed(ApiError::fromJsonError());
        return;
    }

    QString repo_id = dict.value("repo_id").toString();

    emit success(true, repo_id);
}


CreateDefaultRepoRequest::CreateDefaultRepoRequest(const Account& account)
    : SeafileApiRequest (QUrl(account.serverUrl.toString() + kDefaultRepoUrl),
                         SeafileApiRequest::METHOD_POST, account.token)
{
}

void CreateDefaultRepoRequest::requestSuccess(QNetworkReply& reply)
{
    json_error_t error;
    json_t *root = parseJSON(reply, &error);
    if (!root) {
        qDebug("CreateDefaultRepoRequest: failed to parse json:%s\n", error.text);
        emit failed(ApiError::fromJsonError());
        return;
    }

    QScopedPointer<json_t, JsonPointerCustomDeleter> json(root);

    QMap<QString, QVariant> dict = mapFromJSON(json.data(), &error);

    if (!dict.contains("repo_id")) {
        emit failed(ApiError::fromJsonError());
        return;
    }

    emit success(dict.value("repo_id").toString());
}

GetLatestVersionRequest::GetLatestVersionRequest(const QString& client_id,
                                                 const QString& client_version)
    : SeafileApiRequest(QUrl(kLatestVersionUrl), SeafileApiRequest::METHOD_GET)
{
    setParam("id", client_id.left(8));
    setParam("v", QString(kOsName) + "-" + client_version);
}

void GetLatestVersionRequest::requestSuccess(QNetworkReply& reply)
{
    json_error_t error;
    json_t *root = parseJSON(reply, &error);
    if (!root) {
        qDebug("GetLatestVersionRequest: failed to parse json:%s\n", error.text);
        emit failed(ApiError::fromJsonError());
        return;
    }

    QScopedPointer<json_t, JsonPointerCustomDeleter> json(root);

    QMap<QString, QVariant> dict = mapFromJSON(json.data(), &error);

    if (dict.contains(kOsName)) {
        QString version = dict.value(kOsName).toString();
        qDebug("The latest version is %s", toCStr(version));
        emit success(version);
        return;
    }

    emit failed(ApiError::fromJsonError());
}

GetStarredFilesRequest::GetStarredFilesRequest(const Account& account)
    : SeafileApiRequest (QUrl(account.serverUrl.toString() + kStarredFilesUrl),
                         SeafileApiRequest::METHOD_GET, account.token)
{
}

void GetStarredFilesRequest::requestSuccess(QNetworkReply& reply)
{
    json_error_t error;
    json_t *root = parseJSON(reply, &error);
    if (!root) {
        qDebug("GetStarredFilesRequest: failed to parse json:%s\n", error.text);
        emit failed(ApiError::fromJsonError());
        return;
    }

    QScopedPointer<json_t, JsonPointerCustomDeleter> json(root);

    std::vector<StarredFile> files = StarredFile::listFromJSON(json.data(), &error);
    emit success(files);
}

GetEventsRequest::GetEventsRequest(const Account& account, int start)
    : SeafileApiRequest (QUrl(account.serverUrl.toString() + kGetEventsUrl),
                         SeafileApiRequest::METHOD_GET, account.token)
{
    if (start > 0) {
        setParam("start", QString::number(start));
    }
}

void GetEventsRequest::requestSuccess(QNetworkReply& reply)
{
    json_error_t error;
    json_t *root = parseJSON(reply, &error);
    if (!root) {
        qDebug("GetEventsRequest: failed to parse json:%s\n", error.text);
        emit failed(ApiError::fromJsonError());
        return;
    }

    QScopedPointer<json_t, JsonPointerCustomDeleter> json(root);

    bool more = false;
    int more_offset = -1;

    json_t* array = json_object_get(json.data(), "events");
    std::vector<SeafEvent> events = SeafEvent::listFromJSON(array, &error);

    more = json_is_true(json_object_get(json.data(), "more"));
    if (more) {
        more_offset = json_integer_value(json_object_get(json.data(), "more_offset"));
    }

    emit success(events, more_offset);
}

GetCommitDetailsRequest::GetCommitDetailsRequest(const Account& account,
                                           const QString& repo_id,
                                           const QString& commit_id)
    : SeafileApiRequest (QUrl(account.serverUrl.toString() + kCommitDetailsUrl + repo_id + "/"),
                         SeafileApiRequest::METHOD_GET, account.token)
{
    setParam("commit_id", commit_id);
}

void GetCommitDetailsRequest::requestSuccess(QNetworkReply& reply)
{
    json_error_t error;
    json_t *root = parseJSON(reply, &error);
    if (!root) {
        qDebug("GetCommitDetailsRequest: failed to parse json:%s\n", error.text);
        emit failed(ApiError::fromJsonError());
        return;
    }

    QScopedPointer<json_t, JsonPointerCustomDeleter> json(root);

    CommitDetails details = CommitDetails::fromJSON(json.data(), &error);

    emit success(details);
}

// /api2/user/foo@foo.com/resized/36
GetAvatarRequest::GetAvatarRequest(const Account& account,
                                   const QString& email,
                                   int size)
    : SeafileApiRequest (QUrl(account.serverUrl.toString()
                              + kAvatarUrl
                              // + QUrl::toPercentEncoding(email) + "/resized/"
                              + email + "/resized/"
                              + QString::number(size) + "/"),
                         SeafileApiRequest::METHOD_GET, account.token)
{
    account_ = account;
    email_ = email;
    fetch_img_req_ = 0;
}

GetAvatarRequest::~GetAvatarRequest()
{
    if (fetch_img_req_) {
        delete fetch_img_req_;
    }
}

void GetAvatarRequest::requestSuccess(QNetworkReply& reply)
{
    json_error_t error;
    json_t *root = parseJSON(reply, &error);
    if (!root) {
        qDebug("GetAvatarRequest: failed to parse json:%s\n", error.text);
        emit failed(ApiError::fromJsonError());
        return;
    }

    QScopedPointer<json_t, JsonPointerCustomDeleter> json(root);

    const char *avatar_url = json_string_value(json_object_get(json.data(), "url"));

    if (!avatar_url) {
        qDebug("GetAvatarRequest: no 'url' value in response\n");
        emit failed(ApiError::fromJsonError());
        return;
    }

    QString url = QUrl::fromPercentEncoding(avatar_url);

    fetch_img_req_ = new FetchImageRequest(url);

    connect(fetch_img_req_, SIGNAL(failed(const ApiError&)),
            this, SIGNAL(failed(const ApiError&)));
    connect(fetch_img_req_, SIGNAL(success(const QImage&)),
            this, SIGNAL(success(const QImage&)));
    fetch_img_req_->send();
}

FetchImageRequest::FetchImageRequest(const QString& img_url)
    : SeafileApiRequest(QUrl(img_url), SeafileApiRequest::METHOD_GET)
{
}

void FetchImageRequest::requestSuccess(QNetworkReply& reply)
{
    QImage img;
    img.loadFromData(reply.readAll());

    if (img.isNull()) {
        qDebug("FetchImageRequest: invalid image data\n");
        emit failed(ApiError::fromHttpError(400));
    } else {
        emit success(img);
    }
}

SetRepoPasswordRequest::SetRepoPasswordRequest(const Account& account,
                                               const QString& repo_id,
                                               const QString& password)
    : SeafileApiRequest (QUrl(account.serverUrl.toString() + kSetRepoPasswordUrl + repo_id + "/"),
                         SeafileApiRequest::METHOD_POST, account.token)
{
    setParam("password", password);
}

void SetRepoPasswordRequest::requestSuccess(QNetworkReply& reply)
{
    emit success();
}
