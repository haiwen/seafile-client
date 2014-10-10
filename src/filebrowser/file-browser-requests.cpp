#include "file-browser-requests.h"

#include <jansson.h>
#include <QtNetwork>
#include <QScopedPointer>
#include <QDir>
#include <QDebug>

#include "account.h"
#include "api/api-error.h"
#include "seaf-dirent.h"

namespace {

const char kGetDirentsUrl[] = "api2/repos/%1/dir/";
const char kGetFilesUrl[] = "api2/repos/%1/file/";
const char kGetFileSharedLinkUrl[] = "api2/repos/%1/file/shared-link/";
const char kGetFileUploadUrl[] = "api2/repos/%1/upload-link/";
const char kGetFileUpdateUrl[] = "api2/repos/%1/update-link/";
const char kGetStarredFilesUrl[] = "api2/starredfiles/";
//const char kGetFileFromRevisionUrl[] = "api2/repos/%1/file/revision/";
//const char kGetFileDetailUrl[] = "api2/repos/%1/file/detail/";
//const char kGetFileHistoryUrl[] = "api2/repos/%1/file/history/";

} // namespace


GetDirentsRequest::GetDirentsRequest(const Account& account,
                                     const QString& repo_id,
                                     const QString& path,
                                     const QString& cached_dir_id)
    : SeafileApiRequest (account.getAbsoluteUrl(QString(kGetDirentsUrl).arg(repo_id)),
                         SeafileApiRequest::METHOD_GET, account.token)
{
    setParam("p", path);

    // TODO: set the "oid" param
    // if (!cached_dir_id.isEmpty()) {
    //     setParam("oid", cached_dir_id);
    //     cached_dir_id_ = cached_dir_id;
    // }

    repo_id_ = repo_id;
    path_ = path;
}

void GetDirentsRequest::requestSuccess(QNetworkReply& reply)
{
    json_error_t error;
    QList<SeafDirent> dirents;
    QString dir_id = reply.rawHeader("oid");
    if (dir_id.length() != 40) {
        emit failed(ApiError::fromHttpError(500));
        return;
    }

    if (dir_id == cached_dir_id_) {
        emit success(dir_id, dirents);
        return;
    }

    json_t *root = parseJSON(reply, &error);
    if (!root) {
        qDebug("GetDirentsRequest: failed to parse json:%s\n", error.text);
        emit failed(ApiError::fromJsonError());
        return;
    }

    QScopedPointer<json_t, JsonPointerCustomDeleter> json(root);

    dirents = SeafDirent::listFromJSON(json.data(), &error);
    emit success(dir_id, dirents);
}

GetDirentSharedLinkRequest::GetDirentSharedLinkRequest(const Account &account,
                                                       const QString &repo_id,
                                                       const QString &path)
    : SeafileApiRequest(
          account.getAbsoluteUrl(QString(kGetFileSharedLinkUrl).arg(repo_id)),
          SeafileApiRequest::METHOD_GET, account.token)
{
    //TODO use METHOD_PUT, need to hack SeafileApiRequest

    //okay, passing this in http body
    setParam("type", "f");

    //okay, passing this in http body
    setParam("p", path);
}

void GetDirentSharedLinkRequest::requestSuccess(QNetworkReply& reply)
{
    QString reply_content(reply.readAll());

    emit success(reply_content);
}

CreateDirentRequest::CreateDirentRequest(const Account &account,
                                         const QString &repo_id,
                                         const QString &path)
    : SeafileApiRequest(
          account.getAbsoluteUrl(QString(kGetDirentsUrl).arg(repo_id)),
          SeafileApiRequest::METHOD_POST, account.token)
{
    //TODO passing param p in url, need to hack SeafileApiRequest
    setParam("p", path);

    //okay, passing this in post body
    setParam("operation", "mkdir");
}

void CreateDirentRequest::requestSuccess(QNetworkReply& reply)
{
    QString reply_content(reply.readAll());

    emit success(reply_content);
}

RenameDirentRequest::RenameDirentRequest(const Account &account,
                                         const QString &repo_id,
                                         const QString &path,
                                         const QString &new_path)
    : SeafileApiRequest(
          account.getAbsoluteUrl(QString(kGetDirentsUrl).arg(repo_id)),
          SeafileApiRequest::METHOD_POST, account.token)
{
    //TODO passing param p in url, need to hack SeafileApiRequest
    setParam("p", path);

    //okay, passing this in post body
    setParam("operation", "rename");

    //okay, passing this in post body
    setParam("newname", new_path);
}

void RenameDirentRequest::requestSuccess(QNetworkReply& reply)
{
    QString reply_content(reply.readAll());

    emit success(reply_content);
}

RemoveDirentRequest::RemoveDirentRequest(const Account &account,
                                         const QString &repo_id,
                                         const QString &path)
    : SeafileApiRequest(
          account.getAbsoluteUrl(QString(kGetDirentsUrl).arg(repo_id)),
          SeafileApiRequest::METHOD_GET, account.token)
{
    //TODO: Use METHOD_DELETE, need to hack SeafileApiRequest
    setParam("p", path);
}

void RemoveDirentRequest::requestSuccess(QNetworkReply& reply)
{
    QString reply_content(reply.readAll());

    emit success(reply_content);
}

GetFileDownloadRequest::GetFileDownloadRequest(const Account &account,
                                               const QString &repo_id,
                                               const QString &path)
    : SeafileApiRequest(
          account.getAbsoluteUrl(QString(kGetFilesUrl).arg(repo_id)),
          SeafileApiRequest::METHOD_GET, account.token)
{
    setParam("p", path);
}

void GetFileDownloadRequest::requestSuccess(QNetworkReply& reply)
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

        emit success(reply_content, oid);
        return;
    } while (0);
    emit failed(ApiError::fromHttpError(500));
}

GetFileSharedLinkRequest::GetFileSharedLinkRequest(const Account &account,
                                                   const QString &repo_id,
                                                   const QString &path)
    : SeafileApiRequest(
          account.getAbsoluteUrl(QString(kGetFileSharedLinkUrl).arg(repo_id)),
          SeafileApiRequest::METHOD_GET, account.token)
{
    //TODO use METHOD_PUT, need to hack SeafileApiRequest

    //okay, passing this in http body
    setParam("type", "d");

    //okay, passing this in http body
    setParam("p", path);
}

void GetFileSharedLinkRequest::requestSuccess(QNetworkReply& reply)
{
    QString reply_content(reply.readAll());

    emit success(reply_content);
}

GetFileUploadRequest::GetFileUploadRequest(const Account &account,
                                           const QString &repo_id,
                                           const QString &path)
    : SeafileApiRequest(
          account.getAbsoluteUrl(QString(kGetFileUploadUrl).arg(repo_id)),
          SeafileApiRequest::METHOD_GET, account.token)
{
    setParam("p", path);
}

void GetFileUploadRequest::requestSuccess(QNetworkReply& reply)
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

GetFileUpdateRequest::GetFileUpdateRequest(const Account &account,
                                           const QString &repo_id,
                                           const QString &path)
    : SeafileApiRequest(
          account.getAbsoluteUrl(QString(kGetFileUpdateUrl).arg(repo_id)),
          SeafileApiRequest::METHOD_GET, account.token)
{
    setParam("p", path);
}

void GetFileUpdateRequest::requestSuccess(QNetworkReply& reply)
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

RenameFileRequest::RenameFileRequest(const Account &account,
                                     const QString &repo_id,
                                     const QString &path,
                                     const QString &new_name)
    : SeafileApiRequest(
          account.getAbsoluteUrl(QString(kGetFilesUrl).arg(repo_id)),
          SeafileApiRequest::METHOD_POST, account.token)
{
    //TODO passing param p in url, need to hack SeafileApiRequest
    setParam("p", path);

    //okay, passing this in post body
    setParam("operation", "rename");

    //okay, passing this in post body
    setParam("newname", new_name);
}

void RenameFileRequest::requestSuccess(QNetworkReply& reply)
{
    emit success();
}

MoveFileRequest::MoveFileRequest(const Account &account,
                                 const QString &repo_id,
                                 const QString &path,
                                 const QString &new_repo_id,
                                 const QString &new_path)
    : SeafileApiRequest(
          account.getAbsoluteUrl(QString(kGetFilesUrl).arg(repo_id)),
          SeafileApiRequest::METHOD_POST, account.token)
{
    //TODO passing param p in url, need to hack SeafileApiRequest
    setParam("p", path);

    //okay, passing this in post body
    setParam("operation", "move");

    //okay, passing this in post body
    setParam("dst_repo", new_repo_id);

    //okay, passing this in post body
    setParam("dst_dir", new_path);
}

void MoveFileRequest::requestSuccess(QNetworkReply& reply)
{
    emit success();
}

RemoveFileRequest::RemoveFileRequest(const Account &account,
                                     const QString &repo_id,
                                     const QString &path)
    : SeafileApiRequest(
          account.getAbsoluteUrl(QString(kGetFilesUrl).arg(repo_id)),
          SeafileApiRequest::METHOD_GET, account.token)
{
    //TODO: Use METHOD_DELETE, need to hack SeafileApiRequest
    setParam("p", path);
}

void RemoveFileRequest::requestSuccess(QNetworkReply& reply)
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
    //okay, passing this in post body
    setParam("repo_id", repo_id);

    //okay, passing this in post body
    setParam("p", path);
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
          SeafileApiRequest::METHOD_GET, account.token)
{
    //TODO: Use METHOD_DELETE, need to hack SeafileApiRequest
    setParam("repo_id", repo_id);
    setParam("p", path);
}

void UnstarFileRequest::requestSuccess(QNetworkReply& reply)
{
    emit success();
}

