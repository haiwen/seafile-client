#include "file-browser-requests.h"

#include <jansson.h>
#include <QtNetwork>
#include <QScopedPointer>

#include "account.h"
#include "api/api-error.h"
#include "seaf-dirent.h"
#include "utils/utils.h"
#include "utils/file-utils.h"
#include "src/open-local-helper.h"

namespace {

const char kGetDirentsUrl[] = "api2/repos/%1/dir/";
const char kGetFilesUrl[] = "api2/repos/%1/file/";
const char kGetFileSharedLinkUrl[] = "api2/repos/%1/file/shared-link/";
const char kGetFileUploadUrl[] = "api2/repos/%1/upload-link/";
const char kGetFileUpdateUrl[] = "api2/repos/%1/update-link/";
const char kGetStarredFilesUrl[] = "api2/starredfiles/";
const char kFileOperationCopy[] = "api2/repos/%1/fileops/copy/";
const char kAsyncCopyMultipleItems[] = "api/v2.1/repos/async-batch-copy-item/";
const char kAsyncMoveMultipleItems[] = "api/v2.1/repos/async-batch-move-item/";
const char kFileOperationMove[] = "api2/repos/%1/fileops/move/";
const char kRemoveDirentsURL[] = "api2/repos/%1/fileops/delete/";
const char kGetFileUploadedBytesUrl[] = "api/v2.1/repos/%1/file-uploaded-bytes/";
const char kGetSmartLink[] = "api/v2.1/smart-link/";
//const char kGetFileFromRevisionUrl[] = "api2/repos/%1/file/revision/";
//const char kGetFileDetailUrl[] = "api2/repos/%1/file/detail/";
//const char kGetFileHistoryUrl[] = "api2/repos/%1/file/history/";

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
                                           const QString &path,
                                           bool is_file)
    : SeafileApiRequest(
          account.getAbsoluteUrl(QString(kGetFileSharedLinkUrl).arg(repo_id)),
          SeafileApiRequest::METHOD_PUT, account.token), repo_id_(repo_id)
{
    setFormParam("type", is_file ? "f" : "d");
    setFormParam("p", path);
}

void GetSharedLinkRequest::requestSuccess(QNetworkReply& reply)
{
    QString reply_content(reply.rawHeader("Location"));

    emit success(reply_content, repo_id_);
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
        account.getAbsoluteUrl(
            QString(kRemoveDirentsURL).arg(repo_id)),
        SeafileApiRequest::METHOD_POST, account.token),
      repo_id_(repo_id), parent_path_(parent_path), filenames_(filenames)
{
    setUrlParam("p", parent_path_);
    setFormParam("file_names", filenames_.join(":"));
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

AsyncCopyMultipleItemsRequest::AsyncCopyMultipleItemsRequest(const Account &account,
                                                             const QString &repo_id,
                                                             const QString &src_dir_path,
                                                             const QStringList &src_file_names,
                                                             const QString &dst_repo_id,
                                                             const QString &dst_dir_path)
     : SeafileApiRequest(
             account.getAbsoluteUrl(QString(kAsyncCopyMultipleItems)),
             SeafileApiRequest::METHOD_POST, account.token),
       account_(account),
       repo_id_(repo_id),
       src_dir_path_(src_dir_path),
       src_file_names_(src_file_names),
       dst_repo_id_(dst_repo_id),
       dst_repo_path_(dst_dir_path)

{
    setHeader("Content-Type","application/json; charset=utf-8; indent=4");
    setHeader("Accept", "application/json; charset=utf-8; indent=4");

    QJsonObject json_obj;
    QJsonArray json_array;
    json_obj.insert("src_repo_id", repo_id);
    json_obj.insert("src_parent_dir", src_dir_path);
    Q_FOREACH(const QString & src_file_name, src_file_names) {
            json_array.append(src_file_name);
        }
    json_obj.insert("src_dirents", json_array);
    json_obj.insert("dst_repo_id", dst_repo_id);
    json_obj.insert("dst_parent_dir", dst_dir_path);

    QJsonDocument json_document(json_obj);
    QByteArray byte_array = json_document.toJson(QJsonDocument::Compact);
    setRequestBody(byte_array);
    connect(this, SIGNAL(failed(const ApiError&)), this, SLOT(slotInvokeSyncBatchCopyV2(const ApiError&)));
}

void AsyncCopyMultipleItemsRequest::requestSuccess(QNetworkReply& reply)
{
    emit success(dst_repo_id_);
}

void AsyncCopyMultipleItemsRequest::slotInvokeSyncBatchCopyV2(const ApiError& api_error) {
    if (api_error.httpErrorCode() == 404) {
        qWarning("http error code 404, invoke old copy multiple files api");
        req_ = new CopyMultipleFilesRequest(account_, repo_id_, src_dir_path_, src_file_names_,
                                            dst_repo_id_,
                                            dst_repo_path_);
        connect(req_, SIGNAL(success(const QString&)),
            SIGNAL(success(const QString&)));

        connect(req_, SIGNAL(failed(const ApiError&)),
            SIGNAL(failed(const ApiError&)));
        req_->send();
    } else {
        emit sigAsyncCopyMultipleItemsFailed(api_error);
    }
}

AsyncMoveMultipleItemsRequest::AsyncMoveMultipleItemsRequest(const Account &account,
                                                             const QString &repo_id,
                                                             const QString &src_dir_path,
                                                             const QStringList &src_file_names,
                                                             const QString &dst_repo_id,
                                                             const QString &dst_dir_path)
        : SeafileApiRequest(
            account.getAbsoluteUrl(QString(kAsyncMoveMultipleItems)),
            SeafileApiRequest::METHOD_POST, account.token),
          account_(account),
          repo_id_(repo_id),
          src_dir_path_(src_dir_path),
          src_file_names_(src_file_names),
          dst_repo_id_(dst_repo_id),
          dst_repo_path_(dst_dir_path)
{
    setHeader("Content-Type","application/json; charset=utf-8; indent=4");
    setHeader("Accept", "application/json; charset=utf-8; indent=4");

    QJsonObject json_obj;
    QJsonArray json_array;
    json_obj.insert("src_repo_id", repo_id);
    json_obj.insert("src_parent_dir", src_dir_path);
    Q_FOREACH(const QString & src_file_name, src_file_names) {
        json_array.append(src_file_name);
    }
    json_obj.insert("src_dirents", json_array);
    json_obj.insert("dst_repo_id", dst_repo_id);
    json_obj.insert("dst_parent_dir", dst_dir_path);

    QJsonDocument json_document(json_obj);
    QByteArray byte_array = json_document.toJson(QJsonDocument::Compact);
    setRequestBody(byte_array);
    connect(this, SIGNAL(failed(const ApiError&)),
            this, SLOT(slotInvokeSyncBatchMoveV2(const ApiError&)));
}

void AsyncMoveMultipleItemsRequest::requestSuccess(QNetworkReply& reply)
{
    emit success(dst_repo_id_);
}

void AsyncMoveMultipleItemsRequest::slotInvokeSyncBatchMoveV2(const ApiError& api_error) {
    if (api_error.httpErrorCode() == 404) {
        qWarning("http error code 404, invoke old move multiple files api");
        req_ = new MoveMultipleFilesRequest(account_, repo_id_, src_dir_path_, src_file_names_,
                                            dst_repo_id_,
                                            dst_repo_path_);
        connect(req_, SIGNAL(success(const QString&)),
                SIGNAL(success(const QString&)));

        connect(req_, SIGNAL(failed(const ApiError&)),
                SIGNAL(failed(const ApiError&)));
        req_->send();
    } else {
        emit sigAsyncMoveMultipleItemsFailed(api_error);
    }
}

CopyMultipleFilesRequest::CopyMultipleFilesRequest(const Account &account,
                                                   const QString &repo_id,
                                                   const QString &src_dir_path,
                                                   const QStringList &src_file_names,
                                                   const QString &dst_repo_id,
                                                   const QString &dst_dir_path)
    : SeafileApiRequest(
        account.getAbsoluteUrl(QString(kFileOperationCopy).arg(repo_id)),
    SeafileApiRequest::METHOD_POST, account.token),
    repo_id_(repo_id),
    src_dir_path_(src_dir_path),
    src_file_names_(src_file_names),
    dst_repo_id_(dst_repo_id)
{
    setUrlParam("p", src_dir_path);

    setFormParam("file_names", src_file_names.join(":"));
    setFormParam("dst_repo", dst_repo_id);
    setFormParam("dst_dir", dst_dir_path);
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
        account.getAbsoluteUrl(QString(kFileOperationMove).arg(repo_id)),
    SeafileApiRequest::METHOD_POST, account.token),
    repo_id_(repo_id),
    src_dir_path_(src_dir_path),
    src_file_names_(src_file_names),
    dst_repo_id_(dst_repo_id)
{
    setUrlParam("p", src_dir_path);

    setFormParam("file_names", src_file_names.join(":"));
    setFormParam("dst_repo", dst_repo_id);
    setFormParam("dst_dir", dst_dir_path);
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

GetIndexProgressRequest::GetIndexProgressRequest(const QUrl &url, const QString &task_id)
    : SeafileApiRequest(url, SeafileApiRequest::METHOD_GET)
{
    setUrlParam("task_id", task_id);
}

void GetIndexProgressRequest::requestSuccess(QNetworkReply& reply)
{
    json_error_t error;
    json_t *root = parseJSON(reply, &error);
    if (!root) {
        qWarning("GetIndexProgressRequest: failed to parse json:%s\n", error.text);
        emit failed(ApiError::fromJsonError());
        return;
    }

    QScopedPointer<json_t, JsonPointerCustomDeleter> json(root);

    QMap<QString, QVariant> dict = mapFromJSON(json.data(), &error);
    ServerIndexProgress result;

    result.total = dict.value("total").toInt();
    result.indexed = dict.value("indexed").toInt();
    result.status = dict.value("status").toInt();
    emit success(result);
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
      is_dir_(is_dir),
      protocol_link_(OpenLocalHelper::instance()->generateLocalFileSeafileUrl(repo_id, account, path).toEncoded())
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
