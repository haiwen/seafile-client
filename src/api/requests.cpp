#include <jansson.h>

#include <QScopedPointer>
#include <QtNetwork>

#include "account.h"

#include "api-error.h"
#include "commit-details.h"
#include "event.h"
#include "repo-service.h"
#include "rpc/rpc-client.h"
#include "seafile-applet.h"
#include "server-repo.h"
#include "starred-file.h"
#include "utils/api-utils.h"
#include "utils/json-utils.h"
#include "utils/utils.h"

#include "requests.h"

namespace
{
const char* kApiPingUrl = "api2/ping/";
const char* kApiLoginUrl = "api2/auth-token/";
const char* kListReposUrl = "api2/repos/";
const char* kCreateRepoUrl = "api2/repos/";
const char* kGetRepoUrl = "api2/repos/%1/";
const char* kCreateSubrepoUrl = "api2/repos/%1/dir/sub_repo/";
const char* kUnseenMessagesUrl = "api2/unseen_messages/";
const char* kDefaultRepoUrl = "api2/default-repo/";
const char* kStarredFilesUrl = "api2/starredfiles/";
const char* kGetEventsUrl = "api2/events/";
const char* kCommitDetailsUrl = "api2/repo_history_changes/";
const char* kAvatarUrl = "api2/avatars/user/";
const char* kSetRepoPasswordUrl = "api2/repos/";
const char* kServerInfoUrl = "api2/server-info/";
const char* kLogoutDeviceUrl = "api2/logout-device/";
const char* kGetRepoTokensUrl = "api2/repo-tokens/";
const char* kGetLoginTokenUrl = "api2/client-login/";
const char* kFileSearchUrl = "api2/search/";
const char* kAccountInfoUrl = "api2/account/info/";
const char* kDirSharedItemsUrl = "api2/repos/%1/dir/shared_items/";
const char* kFetchGroupsAndContactsUrl = "api2/groupandcontacts/";
const char* kFetchGroupsUrl = "api2/groups/";
const char* kRemoteWipeReportUrl = "api2/device-wiped/";
const char* kSearchUsersUrl = "api2/search-user/";
const char* kSharedLinkUrl = "api/v2.1/share-links/";

const char* kGetThumbnailUrl = "api2/repos/%1/thumbnail/";

} // namespace


PingServerRequest::PingServerRequest(const QUrl& serverAddr)

    : SeafileApiRequest(::urlJoin(serverAddr, kApiPingUrl),
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

    : SeafileApiRequest(::urlJoin(serverAddr, kApiLoginUrl),
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
    json_t* root = parseJSON(reply, &error);
    if (!root) {
        qWarning("failed to parse json:%s\n", error.text);
        emit failed(ApiError::fromJsonError());
        return;
    }

    QScopedPointer<json_t, JsonPointerCustomDeleter> json(root);

    const char* token =
        json_string_value(json_object_get(json.data(), "token"));
    if (token == NULL) {
        qWarning("failed to parse json:%s\n", error.text);
        emit failed(ApiError::fromJsonError());
        return;
    }

    emit success(token);
}


/**
 * ListReposRequest
 */
ListReposRequest::ListReposRequest(const Account& account)
    : SeafileApiRequest(account.getAbsoluteUrl(kListReposUrl),
                        SeafileApiRequest::METHOD_GET,
                        account.token)
{
}

void ListReposRequest::requestSuccess(QNetworkReply& reply)
{
    json_error_t error;
    json_t* root = parseJSON(reply, &error);
    if (!root) {
        qWarning("ListReposRequest:failed to parse json:%s\n", error.text);
        emit failed(ApiError::fromJsonError());
        return;
    }

    QScopedPointer<json_t, JsonPointerCustomDeleter> json(root);

    std::vector<ServerRepo> repos =
        ServerRepo::listFromJSON(json.data(), &error);
    emit success(repos);
}


/**
 * DownloadRepoRequest
 */
DownloadRepoRequest::DownloadRepoRequest(const Account& account,
                                         const QString& repo_id,
                                         bool read_only)
    : SeafileApiRequest(
          account.getAbsoluteUrl("api2/repos/" + repo_id + "/download-info/"),
          SeafileApiRequest::METHOD_GET,
          account.token),
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
    json_t* root = parseJSON(reply, &error);
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
GetRepoRequest::GetRepoRequest(const Account& account, const QString& repoid)
    : SeafileApiRequest(
          account.getAbsoluteUrl(QString(kGetRepoUrl).arg(repoid)),
          SeafileApiRequest::METHOD_GET,
          account.token),
      repoid_(repoid)
{
}

void GetRepoRequest::requestSuccess(QNetworkReply& reply)
{
    json_error_t error;
    json_t* root = parseJSON(reply, &error);
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
CreateRepoRequest::CreateRepoRequest(const Account& account,
                                     const QString& name,
                                     const QString& desc,
                                     const QString& passwd)
    : SeafileApiRequest(account.getAbsoluteUrl(kCreateRepoUrl),
                        SeafileApiRequest::METHOD_POST,
                        account.token)
{
    setFormParam(QString("name"), name);
    setFormParam(QString("desc"), desc);
    if (!passwd.isNull()) {
        qWarning("Encrypted repo");
        setFormParam(QString("passwd"), passwd);
    }
}

CreateRepoRequest::CreateRepoRequest(const Account& account,
                                     const QString& name,
                                     const QString& desc,
                                     int enc_version,
                                     const QString& repo_id,
                                     const QString& magic,
                                     const QString& random_key)
    : SeafileApiRequest(account.getAbsoluteUrl(kCreateRepoUrl),
                        SeafileApiRequest::METHOD_POST,
                        account.token)
{
    setFormParam("name", name);
    setFormParam("desc", desc);
    setFormParam("enc_version", QString::number(enc_version));
    setFormParam("repo_id", repo_id);
    setFormParam("magic", magic);
    setFormParam("random_key", random_key);
}

void CreateRepoRequest::requestSuccess(QNetworkReply& reply)
{
    json_error_t error;
    json_t* root = parseJSON(reply, &error);
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
CreateSubrepoRequest::CreateSubrepoRequest(const Account& account,
                                           const QString& name,
                                           const QString& repoid,
                                           const QString& path,
                                           const QString& passwd)
    : SeafileApiRequest(
          account.getAbsoluteUrl(QString(kCreateSubrepoUrl).arg(repoid)),
          SeafileApiRequest::METHOD_GET,
          account.token)
{
    setUrlParam(QString("p"), path);
    setUrlParam(QString("name"), name);
    if (!passwd.isNull()) {
        setUrlParam(QString("password"), passwd);
    }
}

void CreateSubrepoRequest::requestSuccess(QNetworkReply& reply)
{
    json_error_t error;
    json_t* root = parseJSON(reply, &error);
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
GetUnseenSeahubNotificationsRequest::GetUnseenSeahubNotificationsRequest(
    const Account& account)
    : SeafileApiRequest(account.getAbsoluteUrl(kUnseenMessagesUrl),
                        SeafileApiRequest::METHOD_GET,
                        account.token)
{
}

void GetUnseenSeahubNotificationsRequest::requestSuccess(QNetworkReply& reply)
{
    json_error_t error;
    json_t* root = parseJSON(reply, &error);
    if (!root) {
        qWarning(
            "GetUnseenSeahubNotificationsRequest: failed to parse json:%s\n",
            error.text);
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
    : SeafileApiRequest(account.getAbsoluteUrl(kDefaultRepoUrl),
                        SeafileApiRequest::METHOD_GET,
                        account.token)
{
}

void GetDefaultRepoRequest::requestSuccess(QNetworkReply& reply)
{
    json_error_t error;
    json_t* root = parseJSON(reply, &error);
    if (!root) {
        qWarning("CreateDefaultRepoRequest: failed to parse json:%s\n",
                 error.text);
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
    : SeafileApiRequest(account.getAbsoluteUrl(kDefaultRepoUrl),
                        SeafileApiRequest::METHOD_POST,
                        account.token)
{
}

void CreateDefaultRepoRequest::requestSuccess(QNetworkReply& reply)
{
    json_error_t error;
    json_t* root = parseJSON(reply, &error);
    if (!root) {
        qWarning("CreateDefaultRepoRequest: failed to parse json:%s\n",
                 error.text);
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

GetStarredFilesRequest::GetStarredFilesRequest(const Account& account)
    : SeafileApiRequest(account.getAbsoluteUrl(kStarredFilesUrl),
                        SeafileApiRequest::METHOD_GET,
                        account.token)
{
}

void GetStarredFilesRequest::requestSuccess(QNetworkReply& reply)
{
    json_error_t error;
    json_t* root = parseJSON(reply, &error);
    if (!root) {
        qWarning("GetStarredFilesRequest: failed to parse json:%s\n",
                 error.text);
        emit failed(ApiError::fromJsonError());
        return;
    }

    QScopedPointer<json_t, JsonPointerCustomDeleter> json(root);

    std::vector<StarredFile> files =
        StarredFile::listFromJSON(json.data(), &error);
    emit success(files);
}

GetEventsRequest::GetEventsRequest(const Account& account, int start)
    : SeafileApiRequest(account.getAbsoluteUrl(kGetEventsUrl),
                        SeafileApiRequest::METHOD_GET,
                        account.token)
{
    if (start > 0) {
        setUrlParam("start", QString::number(start));
    }
}

void GetEventsRequest::requestSuccess(QNetworkReply& reply)
{
    json_error_t error;
    json_t* root = parseJSON(reply, &error);
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
        more_offset =
            json_integer_value(json_object_get(json.data(), "more_offset"));
    }

    emit success(events, more_offset);
}

GetCommitDetailsRequest::GetCommitDetailsRequest(const Account& account,
                                                 const QString& repo_id,
                                                 const QString& commit_id)
    : SeafileApiRequest(
          account.getAbsoluteUrl(kCommitDetailsUrl + repo_id + "/"),
          SeafileApiRequest::METHOD_GET,
          account.token)
{
    setUrlParam("commit_id", commit_id);
}

void GetCommitDetailsRequest::requestSuccess(QNetworkReply& reply)
{
    json_error_t error;
    json_t* root = parseJSON(reply, &error);
    if (!root) {
        qWarning("GetCommitDetailsRequest: failed to parse json:%s\n",
                 error.text);
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
                                   qint64 mtime,
                                   int size)
    : SeafileApiRequest(
          account.getAbsoluteUrl(kAvatarUrl + email + "/resized/" +
                                 QString::number(size) + "/"),
          SeafileApiRequest::METHOD_GET,
          account.token),
      fetch_img_req_(NULL),
      mtime_(mtime)
{
    account_ = account;
    email_ = email;
}

GetAvatarRequest::~GetAvatarRequest()
{
    if (fetch_img_req_) {
        fetch_img_req_->deleteLater();
        fetch_img_req_ = nullptr;
    }
}

void GetAvatarRequest::requestSuccess(QNetworkReply& reply)
{
    json_error_t error;
    json_t* root = parseJSON(reply, &error);
    if (!root) {
        qWarning("GetAvatarRequest: failed to parse json:%s\n", error.text);
        emit failed(ApiError::fromJsonError());
        return;
    }

    QScopedPointer<json_t, JsonPointerCustomDeleter> json(root);

    const char* avatar_url =
        json_string_value(json_object_get(json.data(), "url"));

    // we don't need to fetch all images if we have latest one
    json_t* mtime = json_object_get(json.data(), "mtime");
    if (!mtime) {
        qWarning("GetAvatarRequest: no 'mtime' value in response\n");
    }
    else {
        qint64 new_mtime = json_integer_value(mtime);
        if (new_mtime == mtime_) {
            emit success(QImage());
            return;
        }
        mtime_ = new_mtime;
    }

    if (!avatar_url) {
        qWarning("GetAvatarRequest: no 'url' value in response\n");
        emit failed(ApiError::fromJsonError());
        return;
    }

    QString url = QUrl::fromPercentEncoding(avatar_url);

    fetch_img_req_ = new FetchImageRequest(url);

    connect(fetch_img_req_, SIGNAL(failed(const ApiError&)), this,
            SIGNAL(failed(const ApiError&)));
    connect(fetch_img_req_, SIGNAL(success(const QImage&)), this,
            SIGNAL(success(const QImage&)));
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
    }
    else {
        emit success(img);
    }
}

SetRepoPasswordRequest::SetRepoPasswordRequest(const Account& account,
                                               const QString& repo_id,
                                               const QString& password)
    : SeafileApiRequest(
          account.getAbsoluteUrl(kSetRepoPasswordUrl + repo_id + "/"),
          SeafileApiRequest::METHOD_POST,
          account.token)
{
    setFormParam("password", password);
}

void SetRepoPasswordRequest::requestSuccess(QNetworkReply& reply)
{
    emit success();
}

ServerInfoRequest::ServerInfoRequest(const Account& account)
    : SeafileApiRequest(account.getAbsoluteUrl(kServerInfoUrl),
                        SeafileApiRequest::METHOD_GET,
                        account.token),
      account_(account)
{
}

void ServerInfoRequest::requestSuccess(QNetworkReply& reply)
{
    json_error_t error;
    json_t* root = parseJSON(reply, &error);
    if (!root) {
        qWarning("failed to parse json:%s\n", error.text);
        emit failed(ApiError::fromJsonError());
        return;
    }
    QScopedPointer<json_t, JsonPointerCustomDeleter> json(root);

    QMap<QString, QVariant> dict = mapFromJSON(json.data(), &error);

    ServerInfo ret;

    if (dict.contains("version")) {
        ret.parseVersionFromString(dict["version"].toString());
    }

    if (dict.contains("features")) {
        ret.parseFeatureFromStrings(dict["features"].toStringList());
    }

    if (dict.contains("desktop-custom-logo")) {
        ret.customLogo = dict["desktop-custom-logo"].toString();
    }

    if (dict.contains("desktop-custom-brand")) {
        ret.customBrand = dict["desktop-custom-brand"].toString();
    }

    emit success(account_, ret);
}

LogoutDeviceRequest::LogoutDeviceRequest(const Account& account)
    : SeafileApiRequest(account.getAbsoluteUrl(kLogoutDeviceUrl),
                        SeafileApiRequest::METHOD_POST,
                        account.token),
      account_(account)
{
}

void LogoutDeviceRequest::requestSuccess(QNetworkReply& reply)
{
    emit success();
}

GetRepoTokensRequest::GetRepoTokensRequest(const Account& account,
                                           const QStringList& repo_ids,
                                           int batch_size)
    : SeafileApiRequest(account.getAbsoluteUrl(kGetRepoTokensUrl),
                        SeafileApiRequest::METHOD_GET,
                        account.token),
      account_(account),
      repo_ids_(repo_ids),
      batch_offset_(0),
      batch_size_(batch_size)
{
}

void GetRepoTokensRequest::send()
{
    doNextBatch();
}

void GetRepoTokensRequest::doNextBatch()
{
    if (batch_offset_ >= repo_ids_.size()) {
        emit success();
        return;
    }

    int count;
    count = qMin(batch_size_, repo_ids_.size() - batch_offset_);
    batch_req_.reset(new SingleBatchRepoTokensRequest(
        account_, repo_ids_.mid(batch_offset_, count)));

    connect(batch_req_.data(),
            SIGNAL(failed(const ApiError &)),
            this,
            SIGNAL(failed(const ApiError &)));
    connect(batch_req_.data(), SIGNAL(success()), this, SLOT(batchSuccess()));
    batch_req_->send();

    // printf("sending request for one batch: offset = %d, count = %d\n",
    //        batch_offset_,
    //        count);
}

void GetRepoTokensRequest::batchSuccess()
{
    const QMap<QString, QString>& tokens = batch_req_->repoTokens();

    // printf ("one batch finished, offset = %d, count = %d\n", batch_offset_, tokens.size());

    repo_tokens_.unite(tokens);
    batch_offset_ += batch_req_->repoIds().size();
    doNextBatch();
}

void GetRepoTokensRequest::requestSuccess(QNetworkReply& reply)
{
    // Just a place holder. A `GetRepoTokensRequest` is a wrapper around a
    // series of `SingleBatchRepoTokensRequest`, which really sends the api
    // requests.
}

SingleBatchRepoTokensRequest::SingleBatchRepoTokensRequest(const Account& account,
                                           const QStringList& repo_ids)
    : SeafileApiRequest(account.getAbsoluteUrl(kGetRepoTokensUrl),
                        SeafileApiRequest::METHOD_GET,
                        account.token),
      repo_ids_(repo_ids)
{
    setUrlParam("repos", repo_ids.join(","));
}

void SingleBatchRepoTokensRequest::requestSuccess(QNetworkReply& reply)
{
    json_error_t error;
    json_t* root = parseJSON(reply, &error);
    if (!root) {
        qWarning("SingleBatchRepoTokensRequest: failed to parse json:%s\n", error.text);
        emit failed(ApiError::fromJsonError());
        return;
    }

    QScopedPointer<json_t, JsonPointerCustomDeleter> json(root);

    QMap<QString, QVariant> dict = mapFromJSON(json.data(), &error);
    foreach (const QString& repo_id, dict.keys()) {
        repo_tokens_[repo_id] = dict[repo_id].toString();
    }

    emit success();
}

GetLoginTokenRequest::GetLoginTokenRequest(const Account& account,
                                           const QString& next_url)
    : SeafileApiRequest(account.getAbsoluteUrl(kGetLoginTokenUrl),
                        SeafileApiRequest::METHOD_POST,
                        account.token),
      account_(account),
      next_url_(next_url)
{
}

void GetLoginTokenRequest::requestSuccess(QNetworkReply& reply)
{
    json_error_t error;
    json_t* root = parseJSON(reply, &error);
    if (!root) {
        qWarning("GetLoginTokenRequest: failed to parse json:%s\n", error.text);
        emit failed(ApiError::fromJsonError());
        return;
    }

    QScopedPointer<json_t, JsonPointerCustomDeleter> json(root);

    QMap<QString, QVariant> dict = mapFromJSON(json.data(), &error);
    if (!dict.contains("token")) {
        emit failed(ApiError::fromJsonError());
        return;
    }
    emit success(dict["token"].toString());
}

FileSearchRequest::FileSearchRequest(const Account& account,
                                     const QString& keyword,
                                     int page,
                                     int per_page)
    : SeafileApiRequest(account.getAbsoluteUrl(kFileSearchUrl),
                        SeafileApiRequest::METHOD_GET,
                        account.token),
      keyword_(keyword),
      page_(page)
{
    setUrlParam("q", keyword_);
    if (page > 0) {
        setUrlParam("page", QString::number(page));
    }
    // per_page = 2;
    setUrlParam("per_page", QString::number(per_page));
}

void FileSearchRequest::requestSuccess(QNetworkReply& reply)
{
    json_error_t error;
    json_t* root = parseJSON(reply, &error);
    if (!root) {
        qWarning("FileSearchResult: failed to parse json:%s\n", error.text);
        emit failed(ApiError::fromJsonError());
        return;
    }
    QScopedPointer<json_t, JsonPointerCustomDeleter> json(root);
    QMap<QString, QVariant> dict = mapFromJSON(json.data(), &error);
    if (!dict.contains("results")) {
        emit failed(ApiError::fromJsonError());
        return;
    }
    QList<QVariant> results = dict["results"].toList();
    std::vector<FileSearchResult> retval;
    Q_FOREACH(const QVariant& result, results)
    {
        FileSearchResult tmp;
        QMap<QString, QVariant> map = result.toMap();
        if (map.empty())
            continue;
        tmp.repo_id = map["repo_id"].toString();
        tmp.repo_name = RepoService::instance()->getRepo(tmp.repo_id).name;
        tmp.name = map["name"].toString();
        tmp.oid = map["oid"].toString();
        tmp.last_modified = map["last_modified"].toLongLong();
        tmp.fullpath = map["fullpath"].toString();
        tmp.size = map["size"].toLongLong();
        retval.push_back(tmp);
    }
    bool has_more = dict["has_more"].toBool();
    bool is_loading_more = page_ > 1;

    emit success(retval, is_loading_more, has_more);
}

FetchCustomLogoRequest::FetchCustomLogoRequest(const QUrl& url)
    : SeafileApiRequest(url, SeafileApiRequest::METHOD_GET)
{
    setUseCache(true);
}

void FetchCustomLogoRequest::requestSuccess(QNetworkReply& reply)
{
    QPixmap logo;
    logo.loadFromData(reply.readAll());

    if (logo.isNull()) {
        qWarning("FetchCustomLogoRequest: invalid image data\n");
        emit failed(ApiError::fromHttpError(400));
    }
    else {
        emit success(url());
    }
}

FetchAccountInfoRequest::FetchAccountInfoRequest(const Account& account)
    : SeafileApiRequest(account.getAbsoluteUrl(kAccountInfoUrl),
                        SeafileApiRequest::METHOD_GET,
                        account.token)
{
    account_ = account;
}

void FetchAccountInfoRequest::requestSuccess(QNetworkReply& reply)
{
    json_error_t error;
    json_t* root = parseJSON(reply, &error);
    if (!root) {
        qWarning("FetchAccountInfoRequest: failed to parse json:%s\n",
                 error.text);
        emit failed(ApiError::fromJsonError());
        return;
    }

    QScopedPointer<json_t, JsonPointerCustomDeleter> json(root);

    QMap<QString, QVariant> dict = mapFromJSON(json.data(), &error);

    AccountInfo info;
    info.email = dict["email"].toString();
    info.name = dict["name"].toString();
    info.totalStorage = dict["total"].toLongLong();
    info.usedStorage = dict["usage"].toLongLong();
    if (info.name.isEmpty()) {
        info.name = dict["nickname"].toString();
    }
    emit success(info);
}

PrivateShareRequest::PrivateShareRequest(const Account& account,
                                         const QString& repo_id,
                                         const QString& path,
                                         const SeafileUser& user,
                                         int group_id,
                                         SharePermission permission,
                                         ShareType share_type,
                                         ShareOperation op)
    : SeafileApiRequest(
          account.getAbsoluteUrl(QString(kDirSharedItemsUrl).arg(repo_id)),
          op == UPDATE_SHARE ? METHOD_POST : (op == REMOVE_SHARE ? METHOD_DELETE
                                                                 : METHOD_PUT),
          account.token),
      group_id_(share_type == SHARE_TO_GROUP ? group_id : -1),
      user_(share_type == SHARE_TO_USER ? user: SeafileUser()),
      permission_(permission),
      share_type_(share_type),
      share_operation_(op)
{
    setUrlParam("p", path);
    setFormParam("permission", permission == READ_ONLY ? "r" : "rw");
    bool is_add = op == ADD_SHARE;
    if (is_add) {
        setFormParam("share_type",
                     share_type == SHARE_TO_USER ? "user" : "group");
    }
    else {
        setUrlParam("share_type",
                    share_type == SHARE_TO_USER ? "user" : "group");
    }

    if (share_type == SHARE_TO_USER) {
        if (is_add) {
            setFormParam("username", user_.email);
        }
        else {
            setUrlParam("username", user_.email);
        }
    }
    else {
        if (is_add) {
            setFormParam("group_id", QString::number(group_id));
        }
        else {
            setUrlParam("group_id", QString::number(group_id));
        }
    }
}

void PrivateShareRequest::requestSuccess(QNetworkReply& reply)
{
    json_error_t error;
    json_t* root = parseJSON(reply, &error);
    if (!root) {
        qWarning("PrivateShareRequest: failed to parse json:%s\n", error.text);
        emit failed(ApiError::fromJsonError());
        return;
    }

    QScopedPointer<json_t, JsonPointerCustomDeleter> json(root);

    emit success();
}


FetchGroupsAndContactsRequest::FetchGroupsAndContactsRequest(
    const Account& account)
    : SeafileApiRequest(account.getAbsoluteUrl(kFetchGroupsAndContactsUrl),
                        SeafileApiRequest::METHOD_GET,
                        account.token)
{
}

void FetchGroupsAndContactsRequest::requestSuccess(QNetworkReply& reply)
{
    json_error_t error;
    json_t* root = parseJSON(reply, &error);
    if (!root) {
        qWarning("FetchGroupsAndContactsRequest: failed to parse json:%s\n",
                 error.text);
        emit failed(ApiError::fromJsonError());
        return;
    }

    QScopedPointer<json_t, JsonPointerCustomDeleter> json(root);

    QList<SeafileGroup> groups;
    QList<SeafileUser> contacts;

    json_t* groups_array = json_object_get(json.data(), "groups");
    if (groups_array) {
        int i, n = json_array_size(groups_array);
        for (i = 0; i < n; i++) {
            json_t* group_object = json_array_get(groups_array, i);
            const char* name =
                json_string_value(json_object_get(group_object, "name"));
            int group_id =
                json_integer_value(json_object_get(group_object, "id"));
            if (name && group_id) {
                SeafileGroup group;
                group.id = group_id;
                group.name = QString::fromUtf8(name);
                const char* owner =
                    json_string_value(json_object_get(group_object, "creator"));
                if (owner) {
                    group.owner = QString::fromUtf8(owner);
                }
                groups.push_back(group);
            }
        }
    }

    json_t* contacts_array = json_object_get(json.data(), "contacts");
    if (contacts_array) {
        int i, n = json_array_size(contacts_array);

        for (i = 0; i < n; i++) {
            json_t* contact_object = json_array_get(contacts_array, i);
            const char* email =
                json_string_value(json_object_get(contact_object, "email"));
            if (email) {
                SeafileUser contact;
                contact.email = QString::fromUtf8(email);
                contact.name = QString::fromUtf8(
                    json_string_value(json_object_get(contact_object, "name")));
                contacts.push_back(contact);
            }
        }
    }

    emit success(groups, contacts);
}

GetPrivateShareItemsRequest::GetPrivateShareItemsRequest(const Account& account,
                                                         const QString& repo_id,
                                                         const QString& path)
    : SeafileApiRequest(
          account.getAbsoluteUrl(QString(kDirSharedItemsUrl).arg(repo_id)),
          SeafileApiRequest::METHOD_GET,
          account.token)
{
    setUrlParam("p", path);
}

void GetPrivateShareItemsRequest::requestSuccess(QNetworkReply& reply)
{
    json_error_t error;
    json_t* root = parseJSON(reply, &error);
    if (!root) {
        qWarning("GetPrivateShareItemsRequest: failed to parse json:%s\n",
                 error.text);
        emit failed(ApiError::fromJsonError());
        return;
    }

    QScopedPointer<json_t, JsonPointerCustomDeleter> json(root);

    QList<GroupShareInfo> group_shares;
    QList<UserShareInfo> user_shares;

    int i, n = json_array_size(json.data());
    for (i = 0; i < n; i++) {
        json_t* share_info_object = json_array_get(json.data(), i);
        Json share_info(share_info_object);
        QString share_type = share_info.getString("share_type");
        QString permission = share_info.getString("permission");
        if (share_type == "group") {
            // group share
            Json group = share_info.getObject("group_info");
            GroupShareInfo group_share;
            group_share.group.id = group.getLong("id");
            group_share.group.name = group.getString("name");
            group_share.permission = ::permissionfromString(permission);
            group_shares.push_back(group_share);
        }
        else if (share_type == "user") {
            Json user = share_info.getObject("user_info");
            UserShareInfo user_share;
            user_share.user.email = user.getString("name");
            user_share.user.name = user.getString("nickname");
            user_share.permission = ::permissionfromString(permission);
            user_shares.push_back(user_share);
        }
    }

    emit success(group_shares, user_shares);
}

RemoteWipeReportRequest::RemoteWipeReportRequest(const Account& account)
    : SeafileApiRequest(account.getAbsoluteUrl(kRemoteWipeReportUrl),
                        SeafileApiRequest::METHOD_POST)
{
    setFormParam(QString("token"), account.token);
}

void RemoteWipeReportRequest::requestSuccess(QNetworkReply& reply)
{
    emit success();
}

SearchUsersRequest::SearchUsersRequest(
    const Account& account, const QString& pattern)
    : SeafileApiRequest(account.getAbsoluteUrl(kSearchUsersUrl),
                        SeafileApiRequest::METHOD_GET,
                        account.token),
      pattern_(pattern)
{
    setUrlParam("q", pattern);
}

void SearchUsersRequest::requestSuccess(QNetworkReply& reply)
{
    json_error_t error;
    json_t* root = parseJSON(reply, &error);
    if (!root) {
        qWarning("SearchUsersRequest: failed to parse json:%s\n",
                 error.text);
        emit failed(ApiError::fromJsonError());
        return;
    }

    QScopedPointer<json_t, JsonPointerCustomDeleter> json(root);

    QList<SeafileUser> users;

    json_t* users_array = json_object_get(json.data(), "users");
    if (users_array) {
        int i, n = json_array_size(users_array);

        for (i = 0; i < n; i++) {
            json_t* user_object = json_array_get(users_array, i);
            const char* email =
                json_string_value(json_object_get(user_object, "email"));
            if (email) {
                SeafileUser user;
                user.email = QString::fromUtf8(email);
                user.name = QString::fromUtf8(
                    json_string_value(json_object_get(user_object, "name")));
                user.contact_email = QString::fromUtf8(
                    json_string_value(json_object_get(user_object, "contact_email")));
                user.avatar_url = QString::fromUtf8(
                    json_string_value(json_object_get(user_object, "avatar_url")));
                users.push_back(user);
            }
        }
    }

    emit success(users);
}


FetchGroupsRequest::FetchGroupsRequest(
    const Account& account)
    : SeafileApiRequest(account.getAbsoluteUrl(kFetchGroupsUrl),
                        SeafileApiRequest::METHOD_GET,
                        account.token)
{
    setUrlParam("with_msg", "false");
}

void FetchGroupsRequest::requestSuccess(QNetworkReply& reply)
{
    json_error_t error;
    json_t* root = parseJSON(reply, &error);
    if (!root) {
        qWarning("FetchGroupsRequest: failed to parse json:%s\n",
                 error.text);
        emit failed(ApiError::fromJsonError());
        return;
    }

    QScopedPointer<json_t, JsonPointerCustomDeleter> json(root);

    QList<SeafileGroup> groups;

    int i, n = json_array_size(json.data());
    for (i = 0; i < n; i++) {
        json_t* group_object = json_array_get(json.data(), i);
        const char* name =
            json_string_value(json_object_get(group_object, "name"));
        int group_id =
            json_integer_value(json_object_get(group_object, "id"));
        if (name && group_id) {
            SeafileGroup group;
            group.id = group_id;
            group.name = QString::fromUtf8(name);
            const char* owner =
                json_string_value(json_object_get(group_object, "creator"));
            if (owner) {
                group.owner = QString::fromUtf8(owner);
            }
            groups.push_back(group);
        }
    }

    emit success(groups);
}

GetThumbnailRequest::GetThumbnailRequest(const Account& account,
                                         const QString& repo_id,
                                         const QString& path,
					 const QString& dirent_id,
                                         uint size)
    : SeafileApiRequest(
          account.getAbsoluteUrl(QString(kGetThumbnailUrl).arg(repo_id)),
          SeafileApiRequest::METHOD_GET,
          account.token),
      account_(account),
      repo_id_(repo_id),
      path_(path),
      dirent_id_(dirent_id),
      size_(size)
{
    setUrlParam("p", path);
    setUrlParam("size", QString::number(size));
    setUseCache(true);
}

void GetThumbnailRequest::requestSuccess(QNetworkReply& reply)
{
    QPixmap pixmap;
    pixmap.loadFromData(reply.readAll());

    if (pixmap.isNull()) {
        qWarning("GetThumbnailRequest: invalid image data\n");
        emit failed(ApiError::fromHttpError(400));
    }
    else {
        emit success(pixmap);
    }
}

GetSharedLinkRequest::GetSharedLinkRequest(const SharedLinkRequestParams &params)
    : SeafileApiRequest(
          params.account.getAbsoluteUrl(QString(kSharedLinkUrl)),
          SeafileApiRequest::METHOD_GET, params.account.token),
      req_params(params)
{
    setUrlParam("repo_id", req_params.repo_id);
    setUrlParam("path", req_params.path_in_repo);
}

void GetSharedLinkRequest::requestSuccess(QNetworkReply& reply)
{
    SharedLinkInfo shared_link_info;
    json_error_t error;
    json_t* root = parseJSON(reply, &error);
    if (!root) {
        qWarning("GetSharedLinkRequest: failed to parse json:%s\n",
                 error.text);
        emit failed(ApiError::fromJsonError());
        return;
    }

    if (json_array_size(root) == 0) {
        emit empty();
        return;
    }

    QScopedPointer<json_t, JsonPointerCustomDeleter> json(json_array_get(root, 0));
    QMap<QString, QVariant> dict = mapFromJSON(json.data(), &error);

    if (!dict.contains("link")) {
        emit failed(ApiError::fromJsonError());
        return;
    }

    shared_link_info.link = dict.value("link").toString();
    shared_link_info.ctime = dict.value("ctime").toString();
    shared_link_info.expire_date = dict.value("expire_date").toString();
    shared_link_info.is_dir = dict.value("is_dir").toBool();
    shared_link_info.is_expired = dict.value("is_expired").toBool();
    shared_link_info.obj_name = dict.value("obj_name").toString();
    shared_link_info.path = dict.value("path").toString();
    shared_link_info.repo_id = dict.value("repo_id").toString();
    shared_link_info.repo_name = dict.value("repo_name").toString();
    shared_link_info.token = dict.value("token").toString();
    shared_link_info.username = dict.value("username").toString();
    shared_link_info.view_cnt = dict.value("view_cnt").toUInt();

    emit success(shared_link_info);
}

CreateShareLinkRequest::CreateShareLinkRequest(const SharedLinkRequestParams &params,
                                               const QString &password,
                                               quint64 expired_date)
    : SeafileApiRequest(
          params.account.getAbsoluteUrl(QString(kSharedLinkUrl)),
          SeafileApiRequest::METHOD_POST, params.account.token),
      req_params(params)
{
    setFormParam("repo_id", req_params.repo_id);
    setFormParam("path", req_params.path_in_repo);

    SetAdvancedShareParams(password, expired_date);
}

void CreateShareLinkRequest::SetAdvancedShareParams(const QString &password,
                                                    quint64 expired_date)
{
    if (!password.isNull())
        setFormParam("password", password);

    if (expired_date != 0)
        setFormParam("expired_date", QString::number(expired_date));
}

void CreateShareLinkRequest::requestSuccess(QNetworkReply& reply)
{
    SharedLinkInfo shared_link_info;
    json_error_t error;
    json_t* root = parseJSON(reply, &error);
    if (!root) {
        qWarning("CreateShareLinkRequest: failed to parse json:%s\n",
                 error.text);
        emit failed(ApiError::fromJsonError());
        return;
    }

    QScopedPointer<json_t, JsonPointerCustomDeleter> json(root);
    QMap<QString, QVariant> dict = mapFromJSON(json.data(), &error);

    if (!dict.contains("link")) {
        emit failed(ApiError::fromJsonError());
        return;
    }

    shared_link_info.link = dict.value("link").toString();
    shared_link_info.ctime = dict.value("ctime").toString();
    shared_link_info.expire_date = dict.value("expire_date").toString();
    shared_link_info.is_dir = dict.value("is_dir").toBool();
    shared_link_info.is_expired = dict.value("is_expired").toBool();
    shared_link_info.obj_name = dict.value("obj_name").toString();
    shared_link_info.path = dict.value("path").toString();
    shared_link_info.repo_id = dict.value("repo_id").toString();
    shared_link_info.repo_name = dict.value("repo_name").toString();
    shared_link_info.token = dict.value("token").toString();
    shared_link_info.username = dict.value("username").toString();
    shared_link_info.view_cnt = dict.value("view_cnt").toUInt();

    emit success(shared_link_info);
}

DeleteSharedLinkRequest::DeleteSharedLinkRequest(const SharedLinkRequestParams &params,
                                                 const QString &token)
    : SeafileApiRequest(
          params.account.getAbsoluteUrl((QString(kSharedLinkUrl) + "%1/").arg(token)),
          SeafileApiRequest::METHOD_DELETE, params.account.token),
      req_params(params)
{
}

void DeleteSharedLinkRequest::requestSuccess(QNetworkReply& reply)
{
    emit success();
}
