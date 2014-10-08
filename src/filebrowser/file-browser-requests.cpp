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
const char kGetFileDownloadUrl[] = "api2/repos/%1/file/";
const char kGetFileUploadUrl[] = "api2/repos/%1/upload-link/";
//const char kGetFileUpdateUrl[] = "api2/repos/%1/update-link/";
//const char kGetFileStarUrl[] = "api2/starredfiles/";
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

GetFileDownloadRequest::GetFileDownloadRequest(const Account &account,
                                               const QString &repo_id,
                                               const QString &path)
    : SeafileApiRequest(
          account.getAbsoluteUrl(QString(kGetFileDownloadUrl).arg(repo_id)),
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

