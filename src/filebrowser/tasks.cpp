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
#include <QDirIterator>
#include <QTimer>
#include <QApplication>

#include "utils/utils.h"
#include "utils/file-utils.h"
#include "seafile-applet.h"
#include "certs-mgr.h"
#include "api/api-error.h"
#include "configurator.h"
#include "network-mgr.h"

#include "tasks.h"

namespace {

const char *kFileDownloadTmpDirName = "fcachetmp";

const char *kParentDirParam = "form-data; name=\"parent_dir\"";
const char *kTargetFileParam = "form-data; name=\"target_file\"";
const char *kRelativePathParam = "form-data; name=\"relative_path\"";
const char *kFileParamTemplate = "form-data; name=\"file\"; filename=\"%1\"";
const char *kContentTypeApplicationOctetStream = "application/octet-stream";

const int kMaxRedirects = 3;

QNetworkAccessManager *createQNAM() {
    QNetworkAccessManager *manager = new QNetworkAccessManager(qApp);
    NetworkManager::instance()->addWatch(manager);
    return manager;
}

} // namesapce

QThread* FileNetworkTask::worker_thread_;

FileNetworkTask::FileNetworkTask(const Account& account,
                                 const QString& repo_id,
                                 const QString& path,
                                 const QString& local_path)
    : account_(account),
      repo_id_(repo_id),
      path_(path),
      local_path_(local_path),
      canceled_(false),
      progress_(0, 0),
      __shared_ptr(this, &QObject::deleteLater),
      __weak_ptr(__shared_ptr)
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

void FileNetworkTask::onFileServerTaskProgressUpdate(qint64 transferred, qint64 total)
{
    progress_.transferred = transferred;
    progress_.total = total;
    emit progressUpdate(transferred, total);
}

void FileNetworkTask::onLinkGet(const QString& link)
{
    startFileServerTask(link);
}

void FileNetworkTask::startFileServerTask(const QString& link)
{
    createFileServerTask(link);

    connect(fileserver_task_, SIGNAL(progressUpdate(qint64, qint64)),
            this, SLOT(onFileServerTaskProgressUpdate(qint64, qint64)));
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
    __shared_ptr.clear();
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

FileNetworkTask::Progress::Progress(qint64 transferred, qint64 total)
{
    this->transferred = transferred;
    this->total = total;
}

QString FileNetworkTask::Progress::toString() const
{
    if (total > 0) {
        return QString::number(100 * transferred / total) + "%";
    }
    return tr("pending");
}

FileDownloadTask::FileDownloadTask(const Account& account,
                                   const QString& repo_id,
                                   const QString& path,
                                   const QString& local_path,
                                   bool is_save_as_task)
    : FileNetworkTask(account, repo_id, path, local_path), is_save_as_task_(is_save_as_task)
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
                               const QString& local_path,
                               const QString& name,
                               const bool use_upload)
    : FileNetworkTask(account, repo_id, path, local_path),
      name_(name),
      use_upload_(use_upload)
{
}

FileUploadTask::FileUploadTask(const FileUploadTask& rhs)
    : FileNetworkTask(rhs.account_, rhs.repo_id_, rhs.path_, rhs.local_path_),
      name_(rhs.name_),
      use_upload_(rhs.use_upload_)
{
}

void FileUploadTask::createGetLinkRequest()
{
    get_link_req_ = new GetFileUploadLinkRequest(account_, repo_id_, path_, use_upload_);
}

void FileUploadTask::createFileServerTask(const QString& link)
{
    fileserver_task_ = new PostFileTask(link, path_, local_path_,
                                        name_, use_upload_);
}

FileUploadMultipleTask::FileUploadMultipleTask(const Account& account,
                                               const QString& repo_id,
                                               const QString& path,
                                               const QString& local_path,
                                               const QStringList& names,
                                               bool use_upload)
  : FileUploadTask(account, repo_id, path, local_path, QString(), use_upload),
  names_(names)
{
}

void FileUploadMultipleTask::createFileServerTask(const QString& link)
{
    fileserver_task_ = new PostFilesTask(link, path_, local_path_, names_, false);
}

FileUploadDirectoryTask::FileUploadDirectoryTask(const Account& account,
                                                 const QString& repo_id,
                                                 const QString& path,
                                                 const QString& local_path,
                                                 const QString& name)
  : FileUploadTask(account, repo_id, path, local_path, name)
{
}

void FileUploadDirectoryTask::createFileServerTask(const QString& link)
{
    QStringList names;

    if (local_path_ == "/")
        qWarning("attempt to upload the root directory, you should avoid it\n");
    QDir dir(local_path_);
    QDirIterator iterator(dir.absolutePath(), QDirIterator::Subdirectories);
    // XXX (lins05): Move these operations into a thread
    while (iterator.hasNext()) {
        iterator.next();
        QString file_path = iterator.filePath();
        QString relative_path = dir.relativeFilePath(file_path);
        QString base_name = ::getBaseName(file_path);
        if (base_name == "." || base_name == "..") {
            continue;
        }
        if (!iterator.fileInfo().isDir()) {
            names.push_back(relative_path);
        } else {
            if (account_.isAtLeastVersion(4, 4, 0)) {
                // printf("a folder: %s\n", file_path.toUtf8().data());
                if (QDir(file_path).entryList().length() == 2) {
                    // only contains . and .., so an empty folder
                    // printf("an empty folder: %s\n", file_path.toUtf8().data());
                    empty_subfolders_.append(::pathJoin(::getBaseName(local_path_), relative_path));
                }
            }
        }
    }

    // printf("total empty folders: %d for %s\n", empty_subfolders_.length(), dir.absolutePath().toUtf8().data());
    fileserver_task_ = new PostFilesTask(link, path_, dir.absolutePath(), names, true);
}

void FileUploadDirectoryTask::onFinished(bool success)
{
    if (!success || empty_subfolders_.empty()) {
        FileUploadTask::onFinished(success);
        return;
    }

    nextEmptyFolder();
}

void FileUploadDirectoryTask::nextEmptyFolder()
{
    if (empty_subfolders_.isEmpty()) {
        FileUploadDirectoryTask::onFinished(true);
        return;
    }

    QString folder = empty_subfolders_.takeFirst();
    create_dir_req_.reset(new CreateDirectoryRequest(
                              account_, repo_id_, ::pathJoin(path_, folder), true));
    connect(create_dir_req_.data(), SIGNAL(success()),
            this, SLOT(nextEmptyFolder()));
    connect(create_dir_req_.data(), SIGNAL(failed(const ApiError&)),
            this, SLOT(onCreateDirFailed(const ApiError&)));
    create_dir_req_->send();
}

void FileUploadDirectoryTask::onCreateDirFailed(const ApiError &error)
{
    error_ = ApiRequestError;
    error_string_ = error.toString();
    http_error_code_ = error.httpErrorCode();
    FileUploadDirectoryTask::onFinished(false);
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

QNetworkAccessManager *FileServerTask::getQNAM()
{
    if (!network_mgr_ ||
        network_mgr_->networkAccessible() !=
            QNetworkAccessManager::Accessible) {
        network_mgr_ = createQNAM();
    }

    return network_mgr_;
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
        qWarning("[file server task] network error: %s\n", toCStr(reply_->errorString()));
        setError(FileNetworkTask::ApiRequestError, reply_->errorString());
        emit finished(false);
        return;
    }

    if (handleHttpRedirect()) {
        return;
    }

    if ((code / 100) == 4 || (code / 100) == 5) {
        qWarning("request failed for %s: status code %d\n",
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
        qWarning("too many redirects for %s\n",
               toCStr(reply_->url().toString()));
        return true;
    }

    url_ = redirect_attr.toUrl();
    if (url_.isRelative()) {
        url_ = reply_->url().resolved(url_);
    }
    qWarning("redirect to %s (from %s)\n", toCStr(url_.toString()),
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
    reply_ = getQNAM()->get(request);

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

PostFileTask::PostFileTask(const QUrl& url,
                           const QString& parent_dir,
                           const QString& local_path,
                           const QString& name,
                           const QString& relative_path)
    : FileServerTask(url, local_path),
      parent_dir_(parent_dir),
      name_(name),
      use_upload_(true),
      relative_path_(relative_path)
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
        parentdir_part.setBody(parent_dir_.toUtf8());
    } else {
        parentdir_part.setHeader(QNetworkRequest::ContentDispositionHeader,
                                 kTargetFileParam);
        parentdir_part.setBody(::pathJoin(parent_dir_, name_).toUtf8());
    }
    multipart->append(parentdir_part);

    // relative_path param
    if (!relative_path_.isEmpty()) {
        QHttpPart part;
        part.setHeader(QNetworkRequest::ContentDispositionHeader,
                       kRelativePathParam);
        part.setBody(relative_path_.toUtf8());
        multipart->append(part);
    }

    // "file" param
    file_part.setHeader(QNetworkRequest::ContentDispositionHeader,
                        QString(kFileParamTemplate).arg(name_).toUtf8());
    file_part.setHeader(QNetworkRequest::ContentTypeHeader,
                        kContentTypeApplicationOctetStream);
    file_part.setBodyDevice(file_);

    multipart->append(file_part);

    QNetworkRequest request(url_);
    request.setRawHeader("Content-Type",
                         "multipart/form-data; boundary=" + multipart->boundary());
    reply_ = getQNAM()->post(request, multipart);
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

PostFilesTask::PostFilesTask(const QUrl& url,
                             const QString& parent_dir,
                             const QString& local_path,
                             const QStringList& names,
                             const bool use_relative)
    : FileServerTask(url, local_path),
      // work around with server
      parent_dir_(parent_dir.endsWith('/') ? parent_dir : parent_dir + "/"),
      name_(QFileInfo(local_path_).fileName()),
      names_(names),
      current_num_(-1),
      progress_update_timer_(new QTimer(this)),
      use_relative_(use_relative)
{
    // never used, set it to NULL to avoid segment fault
    reply_ = NULL;
    connect(progress_update_timer_, SIGNAL(timeout()), this, SLOT(onProgressUpdate()));
}

PostFilesTask::~PostFilesTask()
{
}

void PostFilesTask::prepare()
{
    current_bytes_ = 0;
    transferred_bytes_ = 0;
    total_bytes_ = 0;

    file_sizes.reserve(names_.size());
    Q_FOREACH(const QString &name, names_)
    {
        QString local_path = ::pathJoin(local_path_, name);
        // approximate the bytes used by http protocol (e.g. the bytes of
        // header)
        qint64 file_size = QFileInfo(local_path).size() + 1024;
        file_sizes.push_back(file_size);
        total_bytes_ += file_size;
    }
}

void PostFilesTask::cancel()
{
    if (canceled_) {
        return;
    }
    progress_update_timer_->stop();
    canceled_ = true;
    task_->cancel();
}

void PostFilesTask::sendRequest()
{
    startNext();
}

void PostFilesTask::onProgressUpdate()
{
    emit progressUpdate(current_bytes_ + transferred_bytes_, total_bytes_);
}

void PostFilesTask::onPostTaskProgressUpdate(qint64 bytes, qint64 /* sum_bytes */)
{
    current_bytes_ = bytes;
}

void PostFilesTask::onPostTaskFinished(bool success)
{
    if (canceled_) {
        return;
    }

    if (!success) {
        error_ = task_->error();
        error_string_ = task_->errorString();
        http_error_code_ = task_->httpErrorCode();
        progress_update_timer_->stop();
        emit finished(false);
        return;
    }

    transferred_bytes_ += file_sizes[current_num_];
    startNext();
}

void PostFilesTask::startNext()
{
    progress_update_timer_->stop();
    if (++current_num_ == names_.size()) {
        emit finished(true);
        return;
    }
    const QString& file_path = names_[current_num_];
    QString file_name = QFileInfo(file_path).fileName();
    QString relative_path;
    if (use_relative_)
        relative_path = ::pathJoin(QFileInfo(local_path_).fileName(), ::getParentPath(file_path));

    // relative_path might be empty, and should be safe to use as well
    task_.reset(new PostFileTask(url_,
        parent_dir_,
        ::pathJoin(local_path_, file_path),
        file_name,
        relative_path));
    connect(task_.data(), SIGNAL(progressUpdate(qint64, qint64)),
            this, SLOT(onPostTaskProgressUpdate(qint64, qint64)));
    connect(task_.data(), SIGNAL(finished(bool)),
            this, SLOT(onPostTaskFinished(bool)));
    current_bytes_ = 0;
    progress_update_timer_->start(100);
    task_->start();
}

void FileServerTask::setError(FileNetworkTask::TaskError error,
                              const QString& error_string)
{
    qWarning("[file server task] error: %s\n", toCStr(error_string));
    error_ = error;
    error_string_ = error_string;
}

void FileServerTask::setHttpError(int code)
{
    error_ = FileNetworkTask::ApiRequestError;
    http_error_code_ = code;
    if (code == 500) {
        error_string_ = tr("Internal Server Error");
    } else if (code == 443 || code == 520) {
        // Handle the case when the storage quote is exceeded. It may happen in two cases:
        //
        // First, the quota has been used up before the upload. In such case
        // seahub would return 520 to the generate-link request.
        // See https://github.com/haiwen/seahub/blob/v6.0.7-server/seahub/api2/views.py#L133
        //
        // Second, the quota is not exceeded before the upload, but would exceed
        // after the upload. In such case httpserver would return 443 to the
        // multipart upload request.
        // See https://github.com/haiwen/seafile-server/blob/v6.0.7-server/server/http-status-codes.h#L10
        error_string_ = tr("The storage quota has been used up");
    }
}
