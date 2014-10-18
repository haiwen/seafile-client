#include <QThread>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryFile>

#include "utils/utils.h"
#include "utils/file-utils.h"
#include "seafile-applet.h"
#include "configurator.h"
#include "file-browser-requests.h"

#include "tasks.h"

namespace {

const char *kFileDownloadTmpDirName = "fcachetmp";

const char *kParentDirParam = "form-data; name=\"parent_dir\"";
const char *kFileParamTemplate = "form-data; name=\"file\"; filename=\"%1\"";
const char *kContentTypeApplicationOctetStream = "application/octet-stream";

const int kMaxRedirects = 3;

} // namesapce

QThread* FileNetworkTask::worker_thread_;

FileNetworkTask::FileNetworkTask(const Account& account,
                                 const QString& repo_id,
                                 const QString& path,
                                 const QString& local_path)
    : account_(account),
      repo_id_(repo_id),
      path_(path),
      local_path_(local_path)
{
    fileserver_task_ = NULL;
    get_link_req_ = NULL;
}

FileNetworkTask::~FileNetworkTask()
{
    if (get_link_req_) {
        delete get_link_req_;
    }
    if (fileserver_task_) {
        delete fileserver_task_;
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
            this, SLOT(onGetLinkFailed()));
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
            this, SLOT(onFinished(bool)));

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
    if (get_link_req_) {
        get_link_req_->deleteLater();
        get_link_req_ = 0;
    }
    if (fileserver_task_) {
        QMetaObject::invokeMethod(fileserver_task_, "cancel");
    }
    onFinished(false);
}

void FileNetworkTask::onFinished(bool success)
{
    emit finished(success);
    deleteLater();
}

void FileNetworkTask::onGetLinkFailed()
{
    onFinished(false);
}

FileDownloadTask::FileDownloadTask(const Account& account,
                                   const QString& repo_id,
                                   const QString& path,
                                   const QString& local_path)
    : FileNetworkTask(account, repo_id, path, local_path)
{
}

void FileDownloadTask::createGetLinkRequest()
{
    get_link_req_ = new GetFileDownloadLinkRequest(account_, repo_id_, path_);
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
                                   const QString& repo_id,
                                   const QString& path,
                                   const QString& local_path)
    : FileNetworkTask(account, repo_id, path, local_path)
{
}

void FileUploadTask::createGetLinkRequest()
{
    get_link_req_ = new GetFileUploadLinkRequest(account_, repo_id_);
}

void FileUploadTask::createFileServerTask(const QString& link)
{
    fileserver_task_ = new PostFileTask(link, path_, local_path_);
}

QNetworkAccessManager* FileServerTask::network_mgr_;

FileServerTask::FileServerTask(const QUrl& url, const QString& local_path)
    : url_(url),
      local_path_(local_path),
      canceled_(false),
      redirect_count_(0)
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
    reply_->ignoreSslErrors();
    // TODO: handle ssl errors
    // emit finished(false);
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

bool FileServerTask::handleHttpRedirect()
{
    QVariant redirect_attr = reply_->attribute(QNetworkRequest::RedirectionTargetAttribute);
    if (redirect_attr.isNull()) {
        return false;
    }

    if (redirect_count_++ > kMaxRedirects) {
        // simply treat too many redirects as server side error
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
        emit finished(false);
        return;
    }

    QString tmpf_path = ::pathJoin(download_tmp_dir, "seaf-XXXXXX");
    tmp_file_ = new QTemporaryFile(tmpf_path);
    tmp_file_->setAutoRemove(false);
    if (!tmp_file_->open()) {
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
            emit finished(false);
        }
    }
}

void GetFileTask::httpRequestFinished()
{
    if (canceled_) {
        return;
    }
    // TODO: check http status code
    tmp_file_->close();

    QString parent_dir = QFileInfo(local_path_).absoluteDir().path();
    if (!::createDirIfNotExists(parent_dir)) {
        emit finished(false);
    }

    if (!tmp_file_->rename(local_path_)) {
        emit finished(false);
        return;
    }

    delete tmp_file_;
    tmp_file_ = 0;
    emit finished(true);
}

PostFileTask::PostFileTask(const QUrl& url,
                           const QString& parent_dir,
                           const QString& local_path)
    : FileServerTask(url, local_path),
      parent_dir_(parent_dir)
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
        emit finished(false);
        return;
    }
    if (!file_->open(QIODevice::ReadOnly)) {
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
    parentdir_part.setHeader(QNetworkRequest::ContentDispositionHeader,
                             kParentDirParam);
    parentdir_part.setBody(toCStr(parent_dir_));

    // "file" param
    QString fname = QFileInfo(local_path_).fileName();
    file_part.setHeader(QNetworkRequest::ContentDispositionHeader,
                        QString(kFileParamTemplate).arg(fname));
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
    connect(reply_, SIGNAL(finished()), this, SLOT(httpRequestFinished()));
    connect(reply_, SIGNAL(uploadProgress(qint64,qint64)),
            this, SIGNAL(progressUpdate(qint64, qint64)));
}

void PostFileTask::httpRequestFinished()
{
    if (canceled_) {
        return;
    }

    if (handleHttpRedirect()) {
        return;
    }

    emit finished(true);
}
