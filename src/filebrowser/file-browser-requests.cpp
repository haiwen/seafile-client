#include <jansson.h>

#include <QtNetwork>
#include <QScopedPointer>

#include "account.h"
#include "api/api-error.h"
#include "seaf-dirent.h"

#include "file-browser-requests.h"

namespace {

const char *kGetDirentsUrl = "api2/repos/%1/dir/";

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
