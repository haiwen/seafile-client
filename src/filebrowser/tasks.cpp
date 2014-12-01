#include <QThread>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryFile>
#include <QSslError>
#include <QSslConfiguration>
#include <QSslCertificate>


#include "utils/utils.h"
#include "utils/file-utils.h"
#include "seafile-applet.h"
#include "certs-mgr.h"
#include "api/api-error.h"
#include "configurator.h"
#include "file-browser-requests.h"

#include "tasks.h"

namespace {

const char *kFileDownloadTmpDirName = "fcachetmp";

const char *kParentDirParam = "form-data; name=\"parent_dir\"";
const char *kTargetFileParam = "form-data; name=\"target_file\"";
const char *kFileParamTemplate = "form-data; name=\"file\"; filename=\"%1\"";
const char *kContentTypeApplicationOctetStream = "application/octet-stream";

const int kMaxRedirects = 3;

} // namesapce

QThread* FileNetworkTask::worker_thread_;

FileNetworkTask::FileNetworkTask(const Account& account,
                                 const ServerRepo& repo,
                                 const QString& path,
                                 const QString& local_path)
    : account_(account),
      repo_(repo),
      path_(path),
      local_path_(local_path),
      canceled_(false)
{
    fileserver_task_ = NULL;
    get_link_req_ = NULL;
    http_error_code_ = 0;
}

FileNetworkTask::~FileNetworkTask()
{
    if (get_link_req_) {
        delete get_link_req_;
    }
    if (fileserver_task_) {
        fileserver_task_->deleteLater();
    }
}

QString FileNetworkTask::fileName() const
{
    return QFileInfo(path_).fileName();
}

void FileNetworkTask::start()
{
    createGetLinkRequest();
    connect(get_link_req_, SIGNAL(success(const QString&)),
            this, SLOT(onLinkGet(const QString&)));
    connect(get_link_req_, SIGNAL(failed(const ApiError&)),
            this, SLOT(onGetLinkFailed(const ApiError&)));
    get_link_req_->send();

}

void FileNetworkTask::onLinkGet(const QString& link)
{
    startFileServerTask(link);
}

void FileNetworkTask::startFileServerTask(const QString& link)
{
    createFileServerTask(link);

    connect(fileserver_task_, SIGNAL(progressUpdate(qint64, qint64)),
            this, SIGNAL(progressUpdate(qint64, qint64)));
    connect(fileserver_task_, SIGNAL(finished(bool)),
            this, SLOT(onFileServerTaskFinished(bool)));

    if (!worker_thread_) {
        worker_thread_ = new QThread;
        worker_thread_->start();
    }
    // From now on the transfer task would run in the worker thread
    fileserver_task_->moveToThread(worker_thread_);
    QMetaObject::invokeMethod(fileserver_task_, "start");
}

void FileNetworkTask::cancel()
{
    canceled_ = true;
    if (get_link_req_) {
        get_link_req_->deleteLater();
        get_link_req_ = 0;
    }
    if (fileserver_task_) {
        QMetaObject::invokeMethod(fileserver_task_, "cancel");
    }
    error_ = TaskCanceled;
    error_string_ = tr("Operation canceled");
    onFinished(false);
}

void FileNetworkTask::onFileServerTaskFinished(bool success)
{
    if (canceled_) {
        return;
    }
    if (!success) {
        error_ = fileserver_task_->error();
        error_string_ = fileserver_task_->errorString();
        http_error_code_ = fileserver_task_->httpErrorCode();
    }
    oid_ = fileserver_task_->oid();
    onFinished(success);
}

void FileNetworkTask::onFinished(bool success)
{
    emit finished(success);
}

void FileNetworkTask::onGetLinkFailed(const ApiError& error)
{
    error_ = ApiRequestError;
    error_string_ = error.toString();
    if (error.type() == ApiError::HTTP_ERROR) {
        http_error_code_ = error.httpErrorCode();
    }
    onFinished(false);
}

FileDownloadTask::FileDownloadTask(const Account& account,
                                   const ServerRepo& repo,
                                   const QString& path,
                                   const QString& local_path)
    : FileNetworkTask(account, repo, path, local_path)
{
}

void FileDownloadTask::createGetLinkRequest()
{
    get_link_req_ = new GetFileDownloadLinkRequest(account_, repo_.id, path_);
}

void FileDownloadTask::onLinkGet(const QString& link)
{
    GetFileDownloadLinkRequest *req = (GetFileDownloadLinkRequest *)get_link_req_;
    file_id_ = req->fileId();
    FileNetworkTask::onLinkGet(link);
}

void FileDownloadTask::createFileServerTask(const QString& link)
{
    fileserver_task_ = new GetFileTask(link, local_path_);
}

FileUploadTask::FileUploadTask(const Account& account,
                               const ServerRepo& repo,
                               const QString& path,
                               const QString& local_path,
                               const QString& name,
                               const bool use_upload)
    : FileNetworkTask(account, repo, path, local_path),
      name_(name),
      use_upload_(use_upload)
{
}

FileUploadTask::FileUploadTask(const FileUploadTask& rhs)
    : FileNetworkTask(rhs.account_, rhs.repo_, rhs.path_, rhs.local_path_),
      name_(rhs.name_),
      use_upload_(rhs.use_upload_)
{
}

void FileUploadTask::createGetLinkRequest()
{
    get_link_req_ = new GetFileUploadLinkRequest(account_, repo_.id, use_upload_);
}

void FileUploadTask::createFileServerTask(const QString& link)
{
    fileserver_task_ = new PostFileTask(link, path_, local_path_,
                                        name_, use_upload_);
}

QNetworkAccessManager* FileServerTask::network_mgr_;

FileServerTask::FileServerTask(const QUrl& url, const QString& local_path)
    : url_(url),
      local_path_(local_path),
      canceled_(false),
      redirect_count_(0),
      http_error_code_(0)
{
}

FileServerTask::~FileServerTask()
{
}

void FileServerTask::onSslErrors(const QList<QSslError>& errors)
{
    if (canceled_) {
        return;
    }
    QUrl url = reply_->url();
    QSslCertificate cert = reply_->sslConfiguration().peerCertificate();
    CertsManager *mgr = seafApplet->certsManager();
    if (!cert.isNull() && cert == mgr->getCertificate(url.toString())) {
        reply_->ignoreSslErrors();
        return;
    }
}

void FileServerTask::start()
{
    prepare();
    sendRequest();
}

void FileServerTask::cancel()
{
    canceled_ = true;
    if (reply_) {
        reply_->abort();
    }
}

void FileServerTask::httpRequestFinished()
{
    int code = reply_->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (code == 0 && reply_->error() != QNetworkReply::NoError) {
        qDebug("[file server task] network error: %s\n", toCStr(reply_->errorString()));
        setError(FileNetworkTask::ApiRequestError, reply_->errorString());
        emit finished(false);
        return;
    }

    if (handleHttpRedirect()) {
        return;
    }

    if ((code / 100) == 4 || (code / 100) == 5) {
        qDebug("request failed for %s: status code %d\n",
               toCStr(reply_->url().toString()), code);
        setHttpError(code);
        emit finished(false);
        return;
    }

    onHttpRequestFinished();
}

bool FileServerTask::handleHttpRedirect()
{
    QVariant redirect_attr = reply_->attribute(QNetworkRequest::RedirectionTargetAttribute);
    if (redirect_attr.isNull()) {
        return false;
    }

    if (redirect_count_++ > kMaxRedirects) {
        // simply treat too many redirects as server side error
        setHttpError(500);
        emit finished(false);
        qDebug("too many redirects for %s\n",
               toCStr(reply_->url().toString()));
        return true;
    }

    url_ = redirect_attr.toUrl();
    if (url_.isRelative()) {
        url_ = reply_->url().resolved(url_);
    }
    qDebug("redirect to %s (from %s)\n", toCStr(url_.toString()),
           toCStr(reply_->url().toString()));
    reply_->deleteLater();
    sendRequest();
    return true;
}


GetFileTask::GetFileTask(const QUrl& url, const QString& local_path)
    : FileServerTask(url, local_path),
      tmp_file_(NULL)
{
}

GetFileTask::~GetFileTask()
{
    if (tmp_file_) {
        tmp_file_->remove();
        delete tmp_file_;
    }
}

void GetFileTask::prepare()
{
    QString download_tmp_dir = ::pathJoin(
        seafApplet->configurator()->seafileDir(), kFileDownloadTmpDirName);
    if (!::createDirIfNotExists(download_tmp_dir)) {
        setError(FileNetworkTask::FileIOError, tr("Failed to create folders"));
        emit finished(false);
        return;
    }

    QString tmpf_path = ::pathJoin(download_tmp_dir, "seaf-XXXXXX");
    tmp_file_ = new QTemporaryFile(tmpf_path);
    tmp_file_->setAutoRemove(false);
    if (!tmp_file_->open()) {
        setError(FileNetworkTask::FileIOError, tr("Failed to create temporary files"));
        emit finished(false);
    }
}

void GetFileTask::sendRequest()
{
    QNetworkRequest request(url_);
    if (!network_mgr_) {
        network_mgr_ = new QNetworkAccessManager;
    }
    reply_ = network_mgr_->get(request);

    connect(reply_, SIGNAL(sslErrors(const QList<QSslError>&)),
            this, SLOT(onSslErrors(const QList<QSslError>&)));

    connect(reply_, SIGNAL(readyRead()), this, SLOT(httpReadyRead()));
    connect(reply_, SIGNAL(downloadProgress(qint64, qint64)),
            this, SIGNAL(progressUpdate(qint64, qint64)));
    connect(reply_, SIGNAL(finished()), this, SLOT(httpRequestFinished()));
}

void GetFileTask::httpReadyRead()
{
    if (canceled_) {
        return;
    }
    // TODO: read in blocks (e.g 64k) instead of readAll
    // TODO: check http status code
    QByteArray chunk = reply_->readAll();
    if (!chunk.isEmpty()) {
        if (tmp_file_->write(chunk) <= 0) {
            setError(FileNetworkTask::FileIOError, tr("Failed to create folders"));
            emit finished(false);
        }
    }
}

void GetFileTask::onHttpRequestFinished()
{
    if (canceled_) {
        return;
    }
    tmp_file_->close();

    QString parent_dir = ::getParentPath(local_path_);
    if (!::createDirIfNotExists(parent_dir)) {
        setError(FileNetworkTask::FileIOError, tr("Failed to write file to disk"));
        emit finished(false);
    }

    QFile oldfile(local_path_);
    if (oldfile.exists() && !oldfile.remove()) {
        setError(FileNetworkTask::FileIOError, tr("Failed to remove the older version of the downloaded file"));
        emit finished(false);
        return;
    }

    if (!tmp_file_->rename(local_path_)) {
        setError(FileNetworkTask::FileIOError, tr("Failed to move file"));
        emit finished(false);
        return;
    }

    delete tmp_file_;
    tmp_file_ = 0;
    emit finished(true);
}

PostFileTask::PostFileTask(const QUrl& url,
                           const QString& parent_dir,
                           const QString& local_path,
                           const QString& name,
                           const bool use_upload)
    : FileServerTask(url, local_path),
      parent_dir_(parent_dir),
      name_(name),
      use_upload_(use_upload)
{
}

PostFileTask::~PostFileTask()
{
}

void PostFileTask::prepare()
{
    file_ = new QFile(local_path_);
    file_->setParent(this);
    if (!file_->exists()) {
        setError(FileNetworkTask::FileIOError, tr("File does not exist"));
        emit finished(false);
        return;
    }
    if (!file_->open(QIODevice::ReadOnly)) {
        setError(FileNetworkTask::FileIOError, tr("File does not exist"));
        emit finished(false);
        return;
    }
}

/**
 * This member function may be called in two places:
 * 1. when task is first started
 * 2. when the request is redirected
 */
void PostFileTask::sendRequest()
{
    QHttpMultiPart *multipart = new QHttpMultiPart(QHttpMultiPart::FormDataType, this);
    // parent_dir param
    QHttpPart parentdir_part, file_part;
    if (use_upload_) {
        parentdir_part.setHeader(QNetworkRequest::ContentDispositionHeader,
                                 kParentDirParam);
        parentdir_part.setBody(toCStr(parent_dir_));
    } else {
        parentdir_part.setHeader(QNetworkRequest::ContentDispositionHeader,
                                 kTargetFileParam);
        parentdir_part.setBody(toCStr(::pathJoin(parent_dir_, name_)));
    }

    // "file" param
    file_part.setHeader(QNetworkRequest::ContentDispositionHeader,
                        QString(kFileParamTemplate).arg(name_).toUtf8());
    file_part.setHeader(QNetworkRequest::ContentTypeHeader,
                        kContentTypeApplicationOctetStream);
    file_part.setBodyDevice(file_);

    multipart->append(parentdir_part);
    multipart->append(file_part);

    QNetworkRequest request(url_);
    request.setRawHeader("Content-Type",
                         "multipart/form-data; boundary=" + multipart->boundary());
    if (!network_mgr_) {
        network_mgr_ = new QNetworkAccessManager;
    }
    reply_ = network_mgr_->post(request, multipart);
    connect(reply_, SIGNAL(sslErrors(const QList<QSslError>&)),
            this, SLOT(onSslErrors(const QList<QSslError>&)));
    connect(reply_, SIGNAL(finished()), this, SLOT(httpRequestFinished()));
    connect(reply_, SIGNAL(uploadProgress(qint64,qint64)),
            this, SIGNAL(progressUpdate(qint64, qint64)));
}

void PostFileTask::onHttpRequestFinished()
{
    if (canceled_) {
        return;
    }

    if (handleHttpRedirect()) {
        return;
    }

    oid_ = reply_->readAll();

    emit finished(true);
}

void FileServerTask::setError(FileNetworkTask::TaskError error,
                              const QString& error_string)
{
    qDebug("[file server task] error: %s\n", toCStr(error_string));
    error_ = error;
    error_string_ = error_string;
}

void FileServerTask::setHttpError(int code)
{
    error_ = FileNetworkTask::ApiRequestError;
    http_error_code_ = code;
    if (code == 500) {
        error_string_ = tr("Internal Server Error");
    }
}
