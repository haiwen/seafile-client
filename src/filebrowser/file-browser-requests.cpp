#include "file-browser-requests.h"

#include <jansson.h>
#include <QtNetwork>
#include <QScopedPointer>

#include "account.h"
#include "api/api-error.h"
#include "seaf-dirent.h"
#include "utils/utils.h"

namespace {

const char kGetDirentsUrl[] = "api2/repos/%1/dir/";
const char kGetFilesUrl[] = "api2/repos/%1/file/";
const char kGetFileSharedLinkUrl[] = "api2/repos/%1/file/shared-link/";
const char kGetFileUploadUrl[] = "api2/repos/%1/upload-link/";
const char kGetFileUpdateUrl[] = "api2/repos/%1/update-link/";
const char kGetStarredFilesUrl[] = "api2/starredfiles/";
const char kFileOperationCopy[] = "api2/repos/%1/fileops/copy/";
const char kFileOperationMove[] = "api2/repos/%1/fileops/move/";
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
        emit failed(ApiError::fromHttpError(500));
        return;
    }
    // this extra header column only supported from v4.2 seahub
    readonly_ = reply.rawHeader("dir_perm") == "r";

    json_t *root = parseJSON(reply, &error);
    if (!root) {
        qDebug("GetDirentsRequest: failed to parse json:%s\n", error.text);
        emit failed(ApiError::fromJsonError());
        return;
    }

    QScopedPointer<json_t, JsonPointerCustomDeleter> json(root);

    QList<SeafDirent> dirents;
    dirents = SeafDirent::listFromJSON(json.data(), &error);
    emit success(readonly_, dirents);
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
    emit success();
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
    emit success();
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
    emit success();
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
    src_file_names_(src_file_names)
{
    setUrlParam("p", src_dir_path);

    setFormParam("file_names", src_file_names.join(":"));
    setFormParam("dst_repo", dst_repo_id);
    setFormParam("dst_dir", dst_dir_path);
}

void CopyMultipleFilesRequest::requestSuccess(QNetworkReply& reply)
{
    emit success();
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
    src_file_names_(src_file_names)
{
    setUrlParam("p", src_dir_path);

    setFormParam("file_names", src_file_names.join(":"));
    setFormParam("dst_repo", dst_repo_id);
    setFormParam("dst_dir", dst_dir_path);
}

void MoveMultipleFilesRequest::requestSuccess(QNetworkReply& reply)
{
    emit success();
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
    emit success();
}
