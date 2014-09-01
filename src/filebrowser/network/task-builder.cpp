#include "task-builder.h"
#include "task.h"
#include "account.h"

namespace {
const char kGetFileUrl[] = "api2/repos/%1/file/";
const char kGetFileFromRevisionUrl[] = "api2/repos/%1/file/revision/";
const char kUploadFileUrl[] = "api2/repos/%1/upload-link/";
const char kUpdateFileUrl[] = "api2/repos/%1/update-link/";
}

SeafileDownloadTask* SeafileNetworkTaskBuilder::createDownloadTask(
                                       const Account &account,
                                       const QString &repo_id,
                                       const QString &path,
                                       const QString &file_name,
                                       const QString &file_location)
{
    QUrl url = account.getAbsoluteUrl(QString(kGetFileUrl).arg(repo_id));
    url.addEncodedQueryItem("p", QUrl::toPercentEncoding(path + file_name));
    return new SeafileDownloadTask(account.token, url, file_name, file_location);
}

SeafileDownloadTask* SeafileNetworkTaskBuilder::createDownloadTask(
                                       const Account &account,
                                       const QString &repo_id,
                                       const QString &path,
                                       const QString &file_name,
                                       const QString &file_location,
                                       const QString &revision)
{
    QUrl url = account.getAbsoluteUrl(QString(kGetFileFromRevisionUrl).arg(repo_id));

    url.addEncodedQueryItem("p", QUrl::toPercentEncoding(path + file_name));
    url.addQueryItem("commit_id", revision);
    return new SeafileDownloadTask(account.token, url, file_name, file_location);
}

SeafileUploadTask* SeafileNetworkTaskBuilder::createUploadTask(
                                       const Account &account,
                                       const QString &repo_id,
                                       const QString &path,
                                       const QString &file_name,
                                       const QString &file_location)
{
    QUrl url = account.getAbsoluteUrl(QString(kUploadFileUrl).arg(repo_id));
    return new SeafileUploadTask(account.token, url, path, file_name, file_location);
}

SeafileUploadTask* SeafileNetworkTaskBuilder::createUpdateTask(
                                       const Account &account,
                                       const QString &repo_id,
                                       const QString &path,
                                       const QString &file_name,
                                       const QString &file_location)
{
    QUrl url = account.getAbsoluteUrl(QString(kUpdateFileUrl).arg(repo_id));
    return new SeafileUploadTask(account.token, url, path, file_name, file_location);
}
