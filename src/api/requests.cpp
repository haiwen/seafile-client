#include <jansson.h>

#include <QtNetwork>
#include <QScopedPointer>

#include "account.h"

#include "seafile-applet.h"
#include "rpc/rpc-client.h"
#include "utils/utils.h"
#include "utils/api-utils.h"
#include "api-error.h"
#include "server-repo.h"
#include "starred-file.h"
#include "event.h"
#include "commit-details.h"

#include "requests.h"

namespace {

const char *kApiPingUrl = "api2/ping/";
const char *kApiLoginUrl = "api2/auth-token/";
const char *kListReposUrl = "api2/repos/";
const char *kCreateRepoUrl = "api2/repos/";
const char *kGetRepoUrl = "api2/repos/%1/";
const char *kCreateSubrepoUrl ="api2/repos/%1/dir/sub_repo/";
const char *kUnseenMessagesUrl = "api2/unseen_messages/";
const char *kDefaultRepoUrl = "api2/default-repo/";
const char *kStarredFilesUrl = "api2/starredfiles/";
const char *kGetEventsUrl = "api2/events/";
const char *kCommitDetailsUrl = "api2/repo_history_changes/";
const char *kAvatarUrl = "api2/avatars/user/";
const char *kSetRepoPasswordUrl = "api2/repos/";
const char *kServerInfoUrl ="api2/server-info/";
const char *kLogoutDeviceUrl = "api2/logout-device/";
const char *kGetRepoTokensUrl = "api2/repo-tokens/";

const char *kLatestVersionUrl = "http://seafile.com/api/client-versions/";

#if defined(Q_OS_WIN32)
const char *kOsName = "windows";
#elif defined(Q_OS_LINUX)
const char *kOsName = "linux";
#else
const char *kOsName = "mac";
#endif

} // namespace


PingServerRequest::PingServerRequest(const QUrl& serverAddr)

    : SeafileApiRequest (::urlJoin(serverAddr, kApiPingUrl),
                         SeafileApiRequest::METHOD_GET)
{
}

void PingServerRequest::requestSuccess(QNetworkReply& reply)
{
    emit success();
}

/**
 * LoginRequest
 */
LoginRequest::LoginRequest(const QUrl& serverAddr,
                           const QString& username,
                           const QString& password,
                           const QString& computer_name)

    : SeafileApiRequest (::urlJoin(serverAddr, kApiLoginUrl),
                         SeafileApiRequest::METHOD_POST)
{
    setFormParam("username", username);
    setFormParam("password", password);

    QHash<QString, QString> params = ::getSeafileLoginParams(computer_name);
    foreach (const QString& key, params.keys()) {
        setFormParam(key, params[key]);
    }
}

void LoginRequest::requestSuccess(QNetworkReply& reply)
{
    json_error_t error;
    json_t *root = parseJSON(reply, &error);
    if (!root) {
        qWarning("failed to parse json:%s\n", error.text);
        emit failed(ApiError::fromJsonError());
        return;
    }

    QScopedPointer<json_t, JsonPointerCustomDeleter> json(root);

    const char *token = json_string_value(json_object_get(json.data(), "token"));
    if (token == NULL) {
        qWarning("failed to parse json:%s\n", error.text);
        emit failed(ApiError::fromJsonError());
        return;
    }

    qWarning("login successful, token is %s\n", token);

    emit success(token);
}


/**
 * ListReposRequest
 */
ListReposRequest::ListReposRequest(const Account& account)
    : SeafileApiRequest (account.getAbsoluteUrl(kListReposUrl),
                         SeafileApiRequest::METHOD_GET, account.token)
{
}

void ListReposRequest::requestSuccess(QNetworkReply& reply)
{
    json_error_t error;
    json_t *root = parseJSON(reply, &error);
    if (!root) {
        qWarning("ListReposRequest:failed to parse json:%s\n", error.text);
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
DownloadRepoRequest::DownloadRepoRequest(const Account& account, const QString& repo_id, bool read_only)
    : SeafileApiRequest(account.getAbsoluteUrl("api2/repos/" + repo_id + "/download-info/"),
                        SeafileApiRequest::METHOD_GET, account.token),
      read_only_(read_only)
{
}

RepoDownloadInfo RepoDownloadInfo::fromDict(QMap<QString, QVariant>& dict,
                                            const QUrl& url_in,
                                            bool read_only)
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
    info.readonly = read_only;

    QUrl url = url_in;
    url.setPath("/");
    info.relay_addr = url.host();

    QMap<QString, QVariant> map;
    map.insert("is_readonly", read_only ? 1 : 0);
    map.insert("server_url", url.toString());

    info.more_info = ::mapToJson(map);

    return info;
}

void DownloadRepoRequest::requestSuccess(QNetworkReply& reply)
{
    json_error_t error;
    json_t *root = parseJSON(reply, &error);
    if (!root) {
        qWarning("failed to parse json:%s\n", error.text);
        emit failed(ApiError::fromJsonError());
        return;
    }

    QScopedPointer<json_t, JsonPointerCustomDeleter> json(root);
    QMap<QString, QVariant> dict = mapFromJSON(json.data(), &error);

    RepoDownloadInfo info = RepoDownloadInfo::fromDict(dict, url(), read_only_);

    emit success(info);
}

/**
 * GetRepoRequest
 */
GetRepoRequest::GetRepoRequest(const Account& account, const QString &repoid)
    : SeafileApiRequest (account.getAbsoluteUrl(QString(kGetRepoUrl).arg(repoid)),
                         SeafileApiRequest::METHOD_GET, account.token)
      , repoid_(repoid)
{
}

void GetRepoRequest::requestSuccess(QNetworkReply& reply)
{
    json_error_t error;
    json_t *root = parseJSON(reply, &error);
    if (!root) {
        qWarning("failed to parse json:%s\n", error.text);
        emit failed(ApiError::fromJsonError());
        return;
    }

    QScopedPointer<json_t, JsonPointerCustomDeleter> json(root);
    QMap<QString, QVariant> dict = mapFromJSON(json.data(), &error);
    ServerRepo repo = ServerRepo::fromJSON(root, &error);

    emit success(repo);
}

/**
 * CreateRepoRequest
 */
CreateRepoRequest::CreateRepoRequest(const Account& account, const QString &name, const QString &desc, const QString &passwd)
    : SeafileApiRequest (account.getAbsoluteUrl(kCreateRepoUrl),
                         SeafileApiRequest::METHOD_POST, account.token)
{
    setFormParam(QString("name"), name);
    setFormParam(QString("desc"), desc);
    if (!passwd.isNull()) {
        qWarning("Encrypted repo");
        setFormParam(QString("passwd"), passwd);
    }
}

void CreateRepoRequest::requestSuccess(QNetworkReply& reply)
{
    json_error_t error;
    json_t *root = parseJSON(reply, &error);
    if (!root) {
        qWarning("failed to parse json:%s\n", error.text);
        emit failed(ApiError::fromJsonError());
        return;
    }

    QScopedPointer<json_t, JsonPointerCustomDeleter> json(root);
    QMap<QString, QVariant> dict = mapFromJSON(json.data(), &error);
    RepoDownloadInfo info = RepoDownloadInfo::fromDict(dict, url(), false);

    emit success(info);
}

/**
 * CreateSubrepoRequest
 */
CreateSubrepoRequest::CreateSubrepoRequest(const Account& account, const QString &name, const QString &repoid , const QString &path, const QString &passwd)
    : SeafileApiRequest (account.getAbsoluteUrl(QString(kCreateSubrepoUrl).arg(repoid)),
                         SeafileApiRequest::METHOD_GET, account.token)
{
    // QString fixed_path = path.left(path.endsWith('/') && path.size() != 1 ? path.size() -1 : path.size());
    setUrlParam(QString("p"), path);
    setUrlParam(QString("name"), name);
    if (!passwd.isNull()) {
        qWarning("Encrypted repo");
        setUrlParam(QString("passwd"), passwd);
    }
}

void CreateSubrepoRequest::requestSuccess(QNetworkReply& reply)
{
    json_error_t error;
    json_t *root = parseJSON(reply, &error);
    if (!root) {
        qWarning("failed to parse json:%s\n", error.text);
        emit failed(ApiError::fromJsonError());
        return;
    }

    QScopedPointer<json_t, JsonPointerCustomDeleter> json(root);
    QMap<QString, QVariant> dict = mapFromJSON(json.data(), &error);

    emit success(dict["sub_repo_id"].toString());
}

/**
 * GetUnseenSeahubNotificationsRequest
 */
GetUnseenSeahubNotificationsRequest::GetUnseenSeahubNotificationsRequest(const Account& account)
    : SeafileApiRequest (account.getAbsoluteUrl(kUnseenMessagesUrl),
                         SeafileApiRequest::METHOD_GET, account.token)
{
}

void GetUnseenSeahubNotificationsRequest::requestSuccess(QNetworkReply& reply)
{
    json_error_t error;
    json_t *root = parseJSON(reply, &error);
    if (!root) {
        qWarning("GetUnseenSeahubNotificationsRequest: failed to parse json:%s\n", error.text);
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
    : SeafileApiRequest (account.getAbsoluteUrl(kDefaultRepoUrl),
                         SeafileApiRequest::METHOD_GET, account.token)
{
}

void GetDefaultRepoRequest::requestSuccess(QNetworkReply& reply)
{
    json_error_t error;
    json_t *root = parseJSON(reply, &error);
    if (!root) {
        qWarning("CreateDefaultRepoRequest: failed to parse json:%s\n", error.text);
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
    : SeafileApiRequest (account.getAbsoluteUrl(kDefaultRepoUrl),
                         SeafileApiRequest::METHOD_POST, account.token)
{
}

void CreateDefaultRepoRequest::requestSuccess(QNetworkReply& reply)
{
    json_error_t error;
    json_t *root = parseJSON(reply, &error);
    if (!root) {
        qWarning("CreateDefaultRepoRequest: failed to parse json:%s\n", error.text);
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
    setUrlParam("id", client_id.left(8));
    setUrlParam("v", QString(kOsName) + "-" + client_version);
}

void GetLatestVersionRequest::requestSuccess(QNetworkReply& reply)
{
    json_error_t error;
    json_t *root = parseJSON(reply, &error);
    if (!root) {
        qWarning("GetLatestVersionRequest: failed to parse json:%s\n", error.text);
        emit failed(ApiError::fromJsonError());
        return;
    }

    QScopedPointer<json_t, JsonPointerCustomDeleter> json(root);

    QMap<QString, QVariant> dict = mapFromJSON(json.data(), &error);

    if (dict.contains(kOsName)) {
        QString version = dict.value(kOsName).toString();
        qWarning("The latest version is %s", toCStr(version));
        emit success(version);
        return;
    }

    emit failed(ApiError::fromJsonError());
}

GetStarredFilesRequest::GetStarredFilesRequest(const Account& account)
    : SeafileApiRequest (account.getAbsoluteUrl(kStarredFilesUrl),
                         SeafileApiRequest::METHOD_GET, account.token)
{
}

void GetStarredFilesRequest::requestSuccess(QNetworkReply& reply)
{
    json_error_t error;
    json_t *root = parseJSON(reply, &error);
    if (!root) {
        qWarning("GetStarredFilesRequest: failed to parse json:%s\n", error.text);
        emit failed(ApiError::fromJsonError());
        return;
    }

    QScopedPointer<json_t, JsonPointerCustomDeleter> json(root);

    std::vector<StarredFile> files = StarredFile::listFromJSON(json.data(), &error);
    emit success(files);
}

GetEventsRequest::GetEventsRequest(const Account& account, int start)
    : SeafileApiRequest (account.getAbsoluteUrl(kGetEventsUrl),
                         SeafileApiRequest::METHOD_GET, account.token)
{
    if (start > 0) {
        setUrlParam("start", QString::number(start));
    }
}

void GetEventsRequest::requestSuccess(QNetworkReply& reply)
{
    json_error_t error;
    json_t *root = parseJSON(reply, &error);
    if (!root) {
        qWarning("GetEventsRequest: failed to parse json:%s\n", error.text);
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
    : SeafileApiRequest (account.getAbsoluteUrl(kCommitDetailsUrl + repo_id + "/"),
                         SeafileApiRequest::METHOD_GET, account.token)
{
    setUrlParam("commit_id", commit_id);
}

void GetCommitDetailsRequest::requestSuccess(QNetworkReply& reply)
{
    json_error_t error;
    json_t *root = parseJSON(reply, &error);
    if (!root) {
        qWarning("GetCommitDetailsRequest: failed to parse json:%s\n", error.text);
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
    : SeafileApiRequest (account.getAbsoluteUrl(
                             kAvatarUrl
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
        qWarning("GetAvatarRequest: failed to parse json:%s\n", error.text);
        emit failed(ApiError::fromJsonError());
        return;
    }

    QScopedPointer<json_t, JsonPointerCustomDeleter> json(root);

    const char *avatar_url = json_string_value(json_object_get(json.data(), "url"));

    if (!avatar_url) {
        qWarning("GetAvatarRequest: no 'url' value in response\n");
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
        qWarning("FetchImageRequest: invalid image data\n");
        emit failed(ApiError::fromHttpError(400));
    } else {
        emit success(img);
    }
}

SetRepoPasswordRequest::SetRepoPasswordRequest(const Account& account,
                                               const QString& repo_id,
                                               const QString& password)
    : SeafileApiRequest (account.getAbsoluteUrl(kSetRepoPasswordUrl + repo_id + "/"),
                         SeafileApiRequest::METHOD_POST, account.token)
{
    setFormParam("password", password);
}

void SetRepoPasswordRequest::requestSuccess(QNetworkReply& reply)
{
    emit success();
}

ServerInfoRequest::ServerInfoRequest(const Account& account)
    : SeafileApiRequest (account.getAbsoluteUrl(kServerInfoUrl),
                         SeafileApiRequest::METHOD_GET, account.token),
    account_(account)
{
}

void ServerInfoRequest::requestSuccess(QNetworkReply& reply)
{
    json_error_t error;
    json_t *root = parseJSON(reply, &error);
    if (!root) {
        qWarning("failed to parse json:%s\n", error.text);
        emit failed(ApiError::fromJsonError());
        return;
    }
    QScopedPointer<json_t, JsonPointerCustomDeleter> json(root);

    QMap<QString, QVariant> dict = mapFromJSON(json.data(), &error);

    ServerInfo ret;

    if (dict.contains("version")) {
        QString versionString = dict["version"].toString();
        QStringList versionList = versionString.split('.');
        if (versionList.size() >= 3) {
            ret.majorVersion = versionList[0].toInt();
            ret.minorVersion = versionList[1].toInt();
            ret.patchVersion = versionList[2].toInt();
        }
    }

    if (dict.contains("features")) {
        QList<QVariant> features = dict["features"].toList();
        Q_FOREACH(const QVariant& feature, features)
        {
            if (feature.toString() == "seafile-pro") {
                ret.pro = true;
                ret.feature |= FeatureProVersion;
            } else if (feature.toString() == "office-preview") {
                ret.feature |= FeatureOfficePreview;
            } else if (feature.toString() == "file-search") {
                ret.feature |= FeatureFileSearch;
            }
        }
    }

    emit success(account_, ret);
}

LogoutDeviceRequest::LogoutDeviceRequest(const Account& account)
    : SeafileApiRequest (account.getAbsoluteUrl(kLogoutDeviceUrl),
                         SeafileApiRequest::METHOD_POST, account.token),
      account_(account)
{
}

void LogoutDeviceRequest::requestSuccess(QNetworkReply& reply)
{
    emit success();
}

GetRepoTokensRequest::GetRepoTokensRequest(const Account& account,
                                           const QStringList& repo_ids)
    : SeafileApiRequest (account.getAbsoluteUrl(kGetRepoTokensUrl),
                         SeafileApiRequest::METHOD_GET, account.token)
{
    setUrlParam("repos", repo_ids.join(","));
}

void GetRepoTokensRequest::requestSuccess(QNetworkReply& reply)
{
    json_error_t error;
    json_t *root = parseJSON(reply, &error);
    if (!root) {
        qWarning("GetRepoTokensRequest: failed to parse json:%s\n", error.text);
        emit failed(ApiError::fromJsonError());
        return;
    }

    QScopedPointer<json_t, JsonPointerCustomDeleter> json(root);

    QMap<QString, QVariant> dict = mapFromJSON(json.data(), &error);
    foreach (const QString &repo_id, dict.keys()) {
        repo_tokens_[repo_id] = dict[repo_id].toString();
    }

    emit success();
}
