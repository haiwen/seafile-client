#include "file-browser-requests.h"

#include <jansson.h>
#include <QtNetwork>
#include <QScopedPointer>
#include <QMapIterator>

#include "account.h"
#include "api/api-error.h"
#include "seaf-dirent.h"
#include "utils/utils.h"
#include "utils/file-utils.h"
#include "src/open-local-helper.h"
#include "utils/json-utils.h"
#include "seafile-applet.h"

namespace {

const char kGetDirentsUrl[] = "api2/repos/%1/dir/";
const char kGetFilesUrl[] = "api2/repos/%1/file/";
const char kGetFileSharedLinkUrl[] = "api/v2.1/share-links/?repo_id=%1&path=%2";
const char kCreateFileSharedLinkUrl[] = "api/v2.1/share-links/";
const char kGetFileUploadUrl[] = "api2/repos/%1/upload-link/";
const char kGetFileUpdateUrl[] = "api2/repos/%1/update-link/";
const char kGetStarredFilesUrl[] = "api2/starredfiles/";
const char kQueryAsyncOperationProgressUrl[] = "api/v2.1/query-copy-move-progress/";
const char kCopyMoveSingleItemUrl[] = "api/v2.1/copy-move-task/";
const char kSyncCopyMultipleItems[] = "api/v2.1/repos/sync-batch-copy-item/";
const char kSyncMoveMultipleItems[] = "api/v2.1/repos/sync-batch-move-item/";
const char kSyncDeleteMultipleItems[] = "api/v2.1/repos/batch-delete-item/";
const char kAsyncCopyMultipleItems[] = "api/v2.1/repos/async-batch-copy-item/";
const char kAsyncMoveMultipleItems[] = "api/v2.1/repos/async-batch-move-item/";
const char kGetFileUploadedBytesUrl[] = "api/v2.1/repos/%1/file-uploaded-bytes/";
const char kGetSmartLink[] = "api/v2.1/smart-link/";
const char kGetUploadLinkUrl[] = "api/v2.1/upload-links/";

//const char kGetFileFromRevisionUrl[] = "api2/repos/%1/file/revision/";
//const char kGetFileDetailUrl[] = "api2/repos/%1/file/detail/";
//const char kGetFileHistoryUrl[] = "api2/repos/%1/file/history/";

QByteArray assembleJsonReq(const QString&  repo_id, const QString& src_dir_path,
                           const QStringList& src_file_names, const QString& dst_repo_id,
                           const QString& dst_dir_path)
{
    QJsonObject json_obj;
    QJsonArray dirents_array;
    json_obj.insert("src_repo_id", repo_id);
    json_obj.insert("src_parent_dir", src_dir_path);
    Q_FOREACH(const QString & src_file_name, src_file_names) {
            dirents_array.append(src_file_name);
    }
    json_obj.insert("src_dirents", dirents_array);
    json_obj.insert("dst_repo_id", dst_repo_id);
    json_obj.insert("dst_parent_dir", dst_dir_path);

    QJsonDocument json_document(json_obj);
    return json_document.toJson(QJsonDocument::Compact);
}

} // namespace


GetDirentsRequest::GetDirentsRequest(const Account& account,
                                     const QString& repo_id,
                                     const QString& path)
    : SeafileApiRequest (account.getAbsoluteUrl(QString(kGetDirentsUrl).arg(repo_id)),
                         SeafileApiRequest::METHOD_GET, account.token),
      repo_id_(repo_id), path_(path), readonly_(false)
{
    setUrlParam("p", path);
}

void GetDirentsRequest::requestSuccess(QNetworkReply& reply)
{
    json_error_t error;
    QString dir_id = reply.rawHeader("oid");
    if (dir_id.length() != 40) {
        emit failed(ApiError::fromHttpError(500), repo_id_);
        return;
    }
    // this extra header column only supported from v4.2 seahub
    readonly_ = reply.rawHeader("dir_perm") == "r";

    json_t *root = parseJSON(reply, &error);
    if (!root) {
        qDebug("GetDirentsRequest: failed to parse json:%s\n", error.text);
        emit failed(ApiError::fromJsonError(), repo_id_);
        return;
    }

    QScopedPointer<json_t, JsonPointerCustomDeleter> json(root);

    QList<SeafDirent> dirents;
    dirents = SeafDirent::listFromJSON(json.data(), &error);
    emit success(readonly_, dirents, repo_id_);
}

GetFileDownloadLinkRequest::GetFileDownloadLinkRequest(const Account &account,
                                                       const QString &repo_id,
                                                       const QString &path)
    : SeafileApiRequest(
          account.getAbsoluteUrl(QString(kGetFilesUrl).arg(repo_id)),
          SeafileApiRequest::METHOD_GET, account.token)
{
    setUrlParam("p", path);
}

void GetFileDownloadLinkRequest::requestSuccess(QNetworkReply& reply)
{
    QString reply_content(reply.readAll());
    QString oid;

    if (reply.hasRawHeader("oid"))
        oid = reply.rawHeader("oid");

    do {
        if (reply_content.size() <= 2)
            break;
        reply_content.remove(0, 1);
        reply_content.chop(1);
        QUrl new_url(reply_content);

        if (!new_url.isValid())
            break;

        file_id_ = oid;
        emit success(reply_content);
        return;
    } while (0);
    emit failed(ApiError::fromHttpError(500));
}

GetSharedLinkRequest::GetSharedLinkRequest(const Account &account,
                                           const QString &repo_id,
                                           const QString &path)
    : SeafileApiRequest(
          account.getAbsoluteUrl(QString(kGetFileSharedLinkUrl).arg(repo_id).arg(path)),
          SeafileApiRequest::METHOD_GET, account.token), repo_id_(repo_id), repo_path_(path)
{
}

void GetSharedLinkRequest::requestSuccess(QNetworkReply& reply)
{
    json_error_t error;
    json_t* root = parseJSON(reply, &error);
    if (!root) {
        qWarning("failed to parse json:%s\n", error.text);
        emit failed(ApiError::fromJsonError());
        return;
    }

    QScopedPointer<json_t, JsonPointerCustomDeleter> json(root);

    const char* share_link = json_string_value(json_object_get(json_array_get(json.data(),0), "link"));
    emit success(share_link);
}


CreateSharedLinkRequest::CreateSharedLinkRequest(const Account &account,
                                                 const QString &repo_id,
                                                 const QString &path,
                                                 const QString &password,
                                                 const QString &expire_days)
        : SeafileApiRequest(
        account.getAbsoluteUrl(QString(kCreateFileSharedLinkUrl)),
        SeafileApiRequest::METHOD_POST, account.token)
{
    QString repo_in_path = QByteArray::fromPercentEncoding(path.toUtf8());
    setFormParam("repo_id", repo_id);
    setFormParam("path", repo_in_path);
    if (!password.isEmpty()) {
        setFormParam("password", password);
    }

    QDateTime now = QDateTime::currentDateTime();
    int offset = now.offsetFromUtc();
    now.setOffsetFromUtc(offset);
    QString target_time = now.addDays(expire_days.toInt()).toString(Qt::ISODate);
    if (!expire_days.isEmpty()) {
        setFormParam("expiration_time", target_time);
    }
}

void CreateSharedLinkRequest::requestSuccess(QNetworkReply& reply)
{
    json_error_t error;
    json_t* root = parseJSON(reply, &error);
    if (!root) {
        qWarning("failed to parse json:%s\n", error.text);
        emit failed(ApiError::fromJsonError());
        return;
    }

    QScopedPointer<json_t, JsonPointerCustomDeleter> json(root);
    const char* share_link = json_string_value(json_object_get(json.data(), "link"));

    emit success(share_link);
}

void CreateSharedLinkRequest::onHttpError(int code)
{
    QJsonDocument doc(QJsonDocument::fromJson(replyBody()));
    error_msg_ = doc["error_msg"].toString();

    emit failed(ApiError::fromHttpError(code));
}

CreateDirectoryRequest::CreateDirectoryRequest(const Account &account,
                                               const QString &repo_id,
                                               const QString &path,
                                               bool create_parents)
    : SeafileApiRequest(
          account.getAbsoluteUrl(QString(kGetDirentsUrl).arg(repo_id)),
          SeafileApiRequest::METHOD_POST, account.token),
      repo_id_(repo_id), path_(path), create_parents_(create_parents)
{
    setUrlParam("p", path);

    setFormParam("operation", "mkdir");
    setFormParam("create_parents", create_parents ? "true" : "false");
}

void CreateDirectoryRequest::requestSuccess(QNetworkReply& reply)
{
    emit success(repo_id_);
}

GetFileUploadLinkRequest::GetFileUploadLinkRequest(const Account &account,
                                                   const QString &repo_id,
                                                   const QString &path,
                                                   bool use_upload)
    : SeafileApiRequest(
          account.getAbsoluteUrl(QString(
              use_upload ? kGetFileUploadUrl : kGetFileUpdateUrl).arg(repo_id)),
          SeafileApiRequest::METHOD_GET, account.token)
{
    setUrlParam("p", path);
}

void GetFileUploadLinkRequest::requestSuccess(QNetworkReply& reply)
{
    QString reply_content(reply.readAll());

    do {
        if (reply_content.size() <= 2)
            break;
        reply_content.remove(0, 1);
        reply_content.chop(1);
        QUrl new_url(reply_content);

        if (!new_url.isValid())
            break;

        emit success(reply_content);
        return;
    } while (0);
    emit failed(ApiError::fromHttpError(500));
}


RenameDirentRequest::RenameDirentRequest(const Account &account,
                                         const QString &repo_id,
                                         const QString &path,
                                         const QString &new_name,
                                         bool is_file)
    : SeafileApiRequest(
        account.getAbsoluteUrl(
            QString(is_file ? kGetFilesUrl: kGetDirentsUrl).arg(repo_id)),
        SeafileApiRequest::METHOD_POST, account.token),
    is_file_(is_file), repo_id_(repo_id), path_(path), new_name_(new_name)
{
    setUrlParam("p", path);

    setFormParam("operation", "rename");
    setFormParam("newname", new_name);
}

void RenameDirentRequest::requestSuccess(QNetworkReply& reply)
{
    emit success(repo_id_);
}

RemoveDirentRequest::RemoveDirentRequest(const Account &account,
                                         const QString &repo_id,
                                         const QString &path,
                                         bool is_file)
    : SeafileApiRequest(
        account.getAbsoluteUrl(
            QString(is_file ? kGetFilesUrl : kGetDirentsUrl).arg(repo_id)),
        SeafileApiRequest::METHOD_DELETE, account.token),
    is_file_(is_file), repo_id_(repo_id), path_(path)
{
    setUrlParam("p", path);
}

void RemoveDirentRequest::requestSuccess(QNetworkReply& reply)
{
    emit success(repo_id_);
}

RemoveDirentsRequest::RemoveDirentsRequest(const Account &account,
                                           const QString &repo_id,
                                           const QString &parent_path,
                                           const QStringList& filenames)
    : SeafileApiRequest(
        account.getAbsoluteUrl(kSyncDeleteMultipleItems),
        SeafileApiRequest::METHOD_DELETE, account.token),
      repo_id_(repo_id), parent_path_(parent_path), filenames_(filenames)
{
    setHeader("Content-Type","application/json");
    setHeader("Accept", "application/json");

    QJsonObject json_obj;
    json_obj.insert("repo_id", repo_id);
    json_obj.insert("parent_dir", parent_path);
    QJsonArray dirents_array;
    for (const QString &filename : filenames) {
        dirents_array.append(filename);
    }
    json_obj.insert("dirents", dirents_array);
    QJsonDocument json_document(json_obj);
    setRequestBody(json_document.toJson(QJsonDocument::Compact));
}

void RemoveDirentsRequest::requestSuccess(QNetworkReply& reply)
{
    emit success(repo_id_);
}


MoveFileRequest::MoveFileRequest(const Account &account,
                                 const QString &repo_id,
                                 const QString &path,
                                 const QString &dst_repo_id,
                                 const QString &dst_dir_path)
    : SeafileApiRequest(
          account.getAbsoluteUrl(QString(kGetFilesUrl).arg(repo_id)),
          SeafileApiRequest::METHOD_POST, account.token)
{
    setUrlParam("p", path);

    setFormParam("operation", "move");
    setFormParam("dst_repo", dst_repo_id);
    setFormParam("dst_dir", dst_dir_path);
}

void MoveFileRequest::requestSuccess(QNetworkReply& reply)
{
    emit success();
}


QueryAsyncOperationProgress::QueryAsyncOperationProgress(const Account &account,
                                                         const QString& task_id)
        : SeafileApiRequest(
        account.getAbsoluteUrl(kQueryAsyncOperationProgressUrl),
        SeafileApiRequest::METHOD_GET, account.token)
{
    setUrlParam("task_id", task_id);
}

void QueryAsyncOperationProgress::requestSuccess(QNetworkReply& reply)
{
    json_error_t error;
    json_t* root = parseJSON(reply, &error);
    if (!root) {
        qWarning("failed to parse json:%s\n", error.text);
        return;
    }

    Json json(root);
    bool is_success = json.getBool("successful");
    bool is_failed = json.getBool("failed");
    if (is_success) {
        emit success();
    } else if (is_failed) {
        qWarning("operation failed");
        emit failed(ApiError::fromHttpError(500));
    }
}


AsyncCopyAndMoveOneItemRequest::AsyncCopyAndMoveOneItemRequest(const Account &account,
                                                               const QString &src_repo_id,
                                                               const QString &src_parent_dir,
                                                               const QString &src_dirent_name,
                                                               const QString &dst_repo_id,
                                                               const QString &dst_parent_dir,
                                                               const QString &operation,
                                                               const QString &dirent_type)
    : SeafileApiRequest(
          account.getAbsoluteUrl(QString(kCopyMoveSingleItemUrl)),
          SeafileApiRequest::METHOD_POST, account.token),
    account_(account),
    repo_id_(src_repo_id),
    src_dir_path_(src_parent_dir),
    src_dirent_name_(src_dirent_name),
    dst_repo_id_(dst_repo_id),
    dst_repo_path_(dst_parent_dir),
    operation_(operation),
    dirent_type_(dirent_type)
{
    setFormParam("src_repo_id", src_repo_id);
    setFormParam("src_parent_dir", src_parent_dir);
    setFormParam("src_dirent_name", src_dirent_name);
    setFormParam("dst_repo_id", dst_repo_id);
    setFormParam("dst_parent_dir", dst_parent_dir);
    setFormParam("operation", operation);
    setFormParam("dirent_type", dirent_type);
}

void AsyncCopyAndMoveOneItemRequest::requestSuccess(QNetworkReply& reply)
{
    json_error_t error;
    json_t* root = parseJSON(reply, &error);
    if (!root) {
        qWarning("failed to parse json:%s\n", error.text);
        return;
    }

    Json json(root);
    QString task_id= json.getString("task_id");
    emit success(task_id);
}


// Asynchronous copy multiple items
AsyncCopyMultipleItemsRequest::AsyncCopyMultipleItemsRequest(const Account &account,
                                                             const QString &repo_id,
                                                             const QString &src_dir_path,
                                                             const QMap<QString, int>&src_dirents,
                                                             const QString &dst_repo_id,
                                                             const QString &dst_dir_path)
     : SeafileApiRequest(
             account.getAbsoluteUrl(QString(kAsyncCopyMultipleItems)),
             SeafileApiRequest::METHOD_POST, account.token),
       account_(account),
       repo_id_(repo_id),
       src_dir_path_(src_dir_path),
       src_dirents_(src_dirents),
       dst_repo_id_(dst_repo_id),
       dst_repo_path_(dst_dir_path)

{

    setHeader("Content-Type","application/json");
    setHeader("Accept", "application/json");

    QStringList file_names;
    for ( const QString & file_name : src_dirents.keys()) {
        file_names.push_back(file_name);
    }
    QByteArray byte_array = assembleJsonReq(repo_id, src_dir_path, file_names,
                                            dst_repo_id, dst_dir_path);
    setRequestBody(byte_array);
}

void AsyncCopyMultipleItemsRequest::requestSuccess(QNetworkReply& reply)
{
    json_error_t error;
    json_t* root = parseJSON(reply, &error);
    if (!root) {
        qWarning("failed to parse json:%s\n", error.text);
        return;
    }

    Json json(root);
    QString task_id = json.getString("task_id");
    emit success(task_id);
}


// Asynchronous api for move multiple items
AsyncMoveMultipleItemsRequest::AsyncMoveMultipleItemsRequest(const Account &account,
                                                             const QString &repo_id,
                                                             const QString &src_dir_path,
                                                             const QMap<QString, int> &src_dirents,
                                                             const QString &dst_repo_id,
                                                             const QString &dst_dir_path)
        : SeafileApiRequest(
            account.getAbsoluteUrl(QString(kAsyncMoveMultipleItems)),
            SeafileApiRequest::METHOD_POST, account.token),
          account_(account),
          repo_id_(repo_id),
          src_dir_path_(src_dir_path),
          src_dirents_(src_dirents),
          dst_repo_id_(dst_repo_id),
          dst_repo_path_(dst_dir_path)
{
    setHeader("Content-Type","application/json");
    setHeader("Accept", "application/json");

    QStringList file_names;
    for ( const QString & file_name : src_dirents.keys()) {
        file_names.push_back(file_name);
    }

    QByteArray byte_array = assembleJsonReq(repo_id, src_dir_path, file_names,
                                            dst_repo_id, dst_dir_path);
    setRequestBody(byte_array);
}

void AsyncMoveMultipleItemsRequest::requestSuccess(QNetworkReply& reply)
{
    json_error_t error;
    json_t* root = parseJSON(reply, &error);
    if (!root) {
        qWarning("failed to parse json:%s\n", error.text);
        return;
    }

    Json json(root);
    QString task_id = json.getString("task_id");
    emit success(task_id);
}


CopyMultipleFilesRequest::CopyMultipleFilesRequest(const Account &account,
                                                   const QString &repo_id,
                                                   const QString &src_dir_path,
                                                   const QStringList &src_file_names,
                                                   const QString &dst_repo_id,
                                                   const QString &dst_dir_path)
    : SeafileApiRequest(
        account.getAbsoluteUrl(kSyncCopyMultipleItems),
    SeafileApiRequest::METHOD_POST, account.token),
    repo_id_(repo_id),
    src_dir_path_(src_dir_path),
    src_file_names_(src_file_names),
    dst_repo_id_(dst_repo_id)
{
    setHeader("Content-Type","application/json");
    setHeader("Accept", "application/json");

    QByteArray byte_array = assembleJsonReq(repo_id, src_dir_path, src_file_names,
                                            dst_repo_id, dst_dir_path);
    setRequestBody(byte_array);
}

void CopyMultipleFilesRequest::requestSuccess(QNetworkReply& reply)
{
    emit success(dst_repo_id_);
}

MoveMultipleFilesRequest::MoveMultipleFilesRequest(const Account &account,
                                                   const QString &repo_id,
                                                   const QString &src_dir_path,
                                                   const QStringList &src_file_names,
                                                   const QString &dst_repo_id,
                                                   const QString &dst_dir_path)
    : SeafileApiRequest(
        account.getAbsoluteUrl(kSyncMoveMultipleItems),
    SeafileApiRequest::METHOD_POST, account.token),
    repo_id_(repo_id),
    src_dir_path_(src_dir_path),
    src_file_names_(src_file_names),
    dst_repo_id_(dst_repo_id)
{
    setHeader("Content-Type","application/json");
    setHeader("Accept", "application/json");

    QByteArray byte_array = assembleJsonReq(repo_id, src_dir_path, src_file_names,
                                            dst_repo_id, dst_dir_path);
    setRequestBody(byte_array);
}

void MoveMultipleFilesRequest::requestSuccess(QNetworkReply& reply)
{
    emit success(dst_repo_id_);
}

StarFileRequest::StarFileRequest(const Account &account,
                                 const QString &repo_id,
                                 const QString &path)
    : SeafileApiRequest(
          account.getAbsoluteUrl(kGetStarredFilesUrl),
          SeafileApiRequest::METHOD_POST, account.token)
{
    setFormParam("repo_id", repo_id);
    setFormParam("p", path);
}

void StarFileRequest::requestSuccess(QNetworkReply& reply)
{
    emit success();
}

UnstarFileRequest::UnstarFileRequest(const Account &account,
                                     const QString &repo_id,
                                     const QString &path)
    : SeafileApiRequest(
          account.getAbsoluteUrl(kGetStarredFilesUrl),
          SeafileApiRequest::METHOD_DELETE, account.token)
{
    setUrlParam("repo_id", repo_id);
    setUrlParam("p", path);
}

void UnstarFileRequest::requestSuccess(QNetworkReply& reply)
{
    emit success();
}

LockFileRequest::LockFileRequest(const Account &account, const QString &repo_id,
                                 const QString &path, bool lock)
    : SeafileApiRequest(
          account.getAbsoluteUrl(QString(kGetFilesUrl).arg(repo_id)),
          SeafileApiRequest::METHOD_PUT, account.token),
      lock_(lock), repo_id_(repo_id), path_(path)
{
    setFormParam("p", path.startsWith("/") ? path : "/" + path);

    setFormParam("operation", lock ? "lock" : "unlock");
}

void LockFileRequest::requestSuccess(QNetworkReply& reply)
{
    emit success(repo_id_);
}

GetFileUploadedBytesRequest::GetFileUploadedBytesRequest(
    const Account &account,
    const QString &repo_id,
    const QString &parent_dir,
    const QString &file_name)
    : SeafileApiRequest(
          account.getAbsoluteUrl(QString(kGetFileUploadedBytesUrl).arg(repo_id)),
          SeafileApiRequest::METHOD_GET,
          account.token),
      repo_id_(repo_id),
      parent_dir_(parent_dir),
      file_name_(file_name)
{
    setUrlParam("parent_dir",
                parent_dir.startsWith("/") ? parent_dir : "/" + parent_dir);
    setUrlParam("file_name", file_name);
}

void GetFileUploadedBytesRequest::requestSuccess(QNetworkReply &reply)
{
    QString accept_ranges_header = reply.rawHeader("Accept-Ranges");
    // printf ("accept_ranges_header = %s\n", toCStr(accept_ranges_header));
    if (accept_ranges_header != "bytes") {
        // Chunked uploading is not supported on the server
        emit success(false, 0);
        return;
    }

    json_error_t error;
    json_t* root = parseJSON(reply, &error);
    if (!root) {
        qWarning("GetFileUploadedBytesRequest: failed to parse json:%s\n",
                 error.text);
        emit failed(ApiError::fromJsonError());
        return;
    }

    QScopedPointer<json_t, JsonPointerCustomDeleter> json(root);

    QMap<QString, QVariant> dict = mapFromJSON(json.data(), &error);
    quint64 uploaded_bytes = dict["uploadedBytes"].toLongLong();
    // printf ("uploadedBytes = %lld\n", uploaded_bytes);
    emit success(true, uploaded_bytes);
}

GetSmartLinkRequest::GetSmartLinkRequest(const Account& account,
                                         const QString &repo_id,
                                         const QString &path,
                                         bool is_dir)
    : SeafileApiRequest(
          account.getAbsoluteUrl(QString(kGetSmartLink)),
          SeafileApiRequest::METHOD_GET, account.token),
      repo_id_(repo_id),
      path_(path),
      protocol_link_(OpenLocalHelper::instance()->generateLocalFileSeafileUrl(repo_id, account, path).toEncoded()),
      is_dir_(is_dir)
{
    setUrlParam("repo_id", repo_id);
    setUrlParam("path", path);
    setUrlParam("is_dir", is_dir ? "true" : "false");
}

void GetSmartLinkRequest::requestSuccess(QNetworkReply& reply)
{
    json_error_t error;
    json_t* root = parseJSON(reply, &error);
    if (!root) {
        qWarning("failed to parse json:%s\n", error.text);
        emit failed(ApiError::fromJsonError());
        return;
    }

    QScopedPointer<json_t, JsonPointerCustomDeleter> json(root);
    const char* smart_link =
        json_string_value(json_object_get(json.data(), "smart_link"));

    emit success(smart_link, protocol_link_);
}

GetFileLockInfoRequest::GetFileLockInfoRequest(const Account& account,
                                               const QString &repo_id,
                                               const QString &path)
    : SeafileApiRequest(
          account.getAbsoluteUrl(QString(kGetDirentsUrl)),
          SeafileApiRequest::METHOD_GET, account.token),
      path_(path)
{
    // Seahub doesn't provide a standalone api for getting file lock
    // info. We have to get that from dirents api.
    dirents_req_.reset(
        new GetDirentsRequest(account, repo_id, ::getParentPath(path_)));
    connect(dirents_req_.data(),
            SIGNAL(success(bool, const QList<SeafDirent> &, const QString &)),
            this,
            SLOT(onGetDirentsSuccess(bool, const QList<SeafDirent> &)));
    connect(dirents_req_.data(),
            SIGNAL(failed(const ApiError &)),
            this,
            SIGNAL(failed(const ApiError &)));
}

void GetFileLockInfoRequest::send()
{
    dirents_req_->send();
}

void GetFileLockInfoRequest::requestSuccess(QNetworkReply& reply)
{
    // Just a place holder. A `GetFileLockInfoRequest` is a wrapper around a
    // `GetDirentsRequest`, which really sends the api
    // requests.
}

void GetFileLockInfoRequest::onGetDirentsSuccess(bool current_readonly, const QList<SeafDirent> &dirents)
{
    const QString name = ::getBaseName(path_);
    foreach(const SeafDirent& dirent, dirents) {
        if (dirent.name == name) {
            const QString lock_owner = dirent.getLockOwnerDisplayString();
            if (!lock_owner.isEmpty()) {
                emit success(true, lock_owner);
            } else {
                emit success(false, "");
            }
            return;
        }
    }
    emit success(false, "");
}

GetUploadLinkRequest::GetUploadLinkRequest(const Account& account,
                                           const QString& repo_id,
                                           const QString& path)
        : SeafileApiRequest(
        account.getAbsoluteUrl(QString(kGetUploadLinkUrl)),
        SeafileApiRequest::METHOD_POST, account.token),
        path_(path)
{
    setFormParam("repo_id", repo_id);
    setFormParam("path", path);
}

void GetUploadLinkRequest::requestSuccess(QNetworkReply& reply)
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
    QString upload_link = dict["link"].toString();
    emit success(upload_link);
}
