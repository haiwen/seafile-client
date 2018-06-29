#include <assert.h>

#include <QBuffer>
#include <QThread>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QFile>
#include <QSslError>
#include <QSslConfiguration>
#include <QSslCertificate>

#include "utils/utils.h"
#include "utils/file-utils.h"
#include "seafile-applet.h"
#include "certs-mgr.h"
#include "api/api-error.h"
#include "configurator.h"
#include "network-mgr.h"
#include "reliable-upload.h"

namespace {

const char *kParentDirParam = "form-data; name=\"parent_dir\"";
const char *kTargetFileParam = "form-data; name=\"target_file\"";
const char *kRelativePathParam = "form-data; name=\"relative_path\"";
const char *kFileParamTemplate = "form-data; name=\"file\"; filename=\"%1\"";
const char *kContentTypeApplicationOctetStream = "application/octet-stream";
const char *kFileNameHeaderTemplate = "attachment; filename=\"%1\"";

// 100MB
const quint32 kMinimalSizeForChunkedUploads = 100 * 1024 * 1024;
// 1MB
const quint32 kUploadChunkSize = 1024 * 1024;

// void printThread(const QString& prefix)
// {
//     printf ("%s %p\n", toCStr(prefix + " "), QThread::currentThreadId());
// }

} // namesapce

ReliablePostFileTask::ReliablePostFileTask(const Account &account,
                                           const QString &repo_id,
                                           const QUrl &url,
                                           const QString &parent_dir,
                                           const QString &local_path,
                                           const QString &name,
                                           const bool use_upload,
                                           const bool accept_user_confirmation)
    : FileServerTask(url, local_path),
      account_(account),
      repo_id_(repo_id),
      part_of_folder_upload_(false),
      parent_dir_(parent_dir),
      file_(nullptr),
      name_(name),
      use_upload_(use_upload)
{
    current_offset_ = 0;
    current_chunk_size_ = 0;
    done_ = 0;
    total_size_ = 0;
    // Chunked uploading is disabled for updating an existing file
    resumable_ = use_upload;
    accept_user_confirmation_ = accept_user_confirmation;
}

ReliablePostFileTask::ReliablePostFileTask(const QUrl &url,
                                           const QString &parent_dir,
                                           const QString &local_path,
                                           const QString &name,
                                           const QString &relative_path)
    : FileServerTask(url, local_path),
      account_(Account()),
      repo_id_(QString()),
      part_of_folder_upload_(true),
      parent_dir_(parent_dir),
      file_(nullptr),
      name_(name),
      use_upload_(true),
      relative_path_(relative_path)
{
    current_offset_ = 0;
    current_chunk_size_ = 0;
    done_ = 0;
    total_size_ = 0;
    resumable_ = false;

    // This constructor is called by the PostFilesTask, which handles user
    // confirmation and retry itself.
    accept_user_confirmation_ = false;
}

ReliablePostFileTask::~ReliablePostFileTask()
{
    if (file_) {
        file_->close();
        file_ = nullptr;
    }
}

bool ReliablePostFileTask::retryEnabled()
{
    return true;
}

void ReliablePostFileTask::prepare()
{
    if (file_) {
        // In case of retry
        delete file_;
    }
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
    total_size_ = file_->size();
}

bool ReliablePostFileTask::useResumableUpload() const
{
    return resumable_ && total_size_ > kMinimalSizeForChunkedUploads;
}

void ReliablePostFileTask::sendRequest()
{
    if (useResumableUpload()) {
        checkUploadedBytes();
    } else {
        startPostFileTask();
    }
}

void ReliablePostFileTask::checkUploadedBytes()
{
    file_uploaded_bytes_req_.reset(
        new GetFileUploadedBytesRequest(account_, repo_id_, parent_dir_, name_));

    connect(file_uploaded_bytes_req_.data(), SIGNAL(success(bool, quint64)),
            this, SLOT(onGetFileUploadedBytesSuccess(bool, quint64)));
    connect(file_uploaded_bytes_req_.data(), SIGNAL(failed(const ApiError&)),
            this, SLOT(onGetFileUploadedBytesFailed(const ApiError&)));
    file_uploaded_bytes_req_->send();
}

void ReliablePostFileTask::onGetFileUploadedBytesSuccess(bool support_chunked_uploading, quint64 uploaded_bytes)
{
    if (!support_chunked_uploading) {
        resumable_ = false;
        current_offset_ = 0;
    } else {
        current_offset_ = uploaded_bytes;
    }

    startPostFileTask();
}

void ReliablePostFileTask::onGetFileUploadedBytesFailed(const ApiError& error)
{
    resumable_ = false;
    current_offset_ = 0;
    // printf("file %s: get uploaded bytes failed with %s\n",
    //        name_.toUtf8().data(),
    //        error.toString().toUtf8().data());
    startPostFileTask();
}

void ReliablePostFileTask::startPostFileTask()
{
    // Update the progress when retrying
    emit progressUpdate(current_offset_, total_size_);

    createPostFileTask();
    QMetaObject::invokeMethod(task_.data(), "start");
}

void ReliablePostFileTask::createPostFileTask()
{
    if (part_of_folder_upload_) {
        // File uploading as part of directory uploading: does not support
        // chunked uploads
        task_.reset(new PostFileTask(url_,
                                     parent_dir_,
                                     local_path_,
                                     file_,
                                     name_,
                                     relative_path_,
                                     total_size_));
    } else {
        QUrl url = url_;
        if (useResumableUpload()) {
            current_chunk_size_ =
                qMin((quint64)kUploadChunkSize, total_size_ - current_offset_);
            // This is a quick fix when the server hasn't implemented chunking
            // support in /upload/api/ endpoint yet.
            url = QUrl(url_.toString(QUrl::PrettyDecoded).replace("/upload-api/", "/upload-aj/"));
            // printf ("old url: %s\n", url_.toEncoded().data());
            // printf ("new url: %s\n", url.toEncoded().data());
            need_idx_progress_ = true;
        } else {
            need_idx_progress_ = false;
        }
        // For emulating upload errors
        // url = QUrl(url_.toString(QUrl::PrettyDecoded).replace("1", "x").replace("2", "x"));
        task_.reset(new PostFileTask(url,
                                     parent_dir_,
                                     local_path_,
                                     file_,
                                     name_,
                                     use_upload_,
                                     total_size_,
                                     current_offset_,
                                     current_chunk_size_,
                                     need_idx_progress_));
    }
    setupSignals();
    // printThread("reliable task is in thread");
    task_.data()->moveToThread(FileNetworkTask::worker_thread_);
}

void ReliablePostFileTask::setupSignals()
{
    connect(task_.data(), SIGNAL(finished(bool)), this, SLOT(onPostFileTaskFinished(bool)));
    connect(task_.data(),
            SIGNAL(progressUpdate(qint64, qint64)),
            this,
            SLOT(onPostFileTaskProgressUpdate(qint64, qint64)));
}

void ReliablePostFileTask::handlePostFileTaskFailure()
{
    if (!canceled_ && accept_user_confirmation_) {
        emit oneFileFailed(name_, true);
    } else {
        emit finished(false);
    }
}

void ReliablePostFileTask::continueWithFailedFile(bool retry, const QString& link)
{
    // retry=false mean skip the current file and continue with the
    // next one. It is only used in PostFilesTask which contains a
    // list of files to be uploaded.
    assert(retry);
    start();
}

void ReliablePostFileTask::onPostFileTaskFinished(bool result)
{
    // First check if we should retry on failure
    if (!result) {
        if (task_->error() == FileNetworkTask::ApiRequestError && maybeRetry()) {
            return;
        }
    }

    http_error_code_ = task_->httpErrorCode();
    error_string_ = task_->errorString();
    error_ = task_->error();

    if (!useResumableUpload()) {
        // Simple upload
        if (result) {
            emit finished(true);
        } else {
            handlePostFileTaskFailure();
        }
        return;
    }

    // printf ("total_size: %llu, offset: %llu, chunk: %u\n", total_size_, current_offset_, current_chunk_size_);

    // Resumable upload
    if (!result) {
        handlePostFileTaskFailure();
        return;
    }

    // A chunk is successfully uploaded to the server
    if (current_offset_ + current_chunk_size_ >= total_size_) {
        emit finished(true);
        return;
    } else {
        current_offset_ += current_chunk_size_;
        // Continue with next chunk
        startPostFileTask();
    }
}

void ReliablePostFileTask::onPostFileTaskProgressUpdate(qint64 done, qint64 total)
{
    done_ = current_offset_ + done;
    emit progressUpdate(done_, total_size_);
}

const QString& ReliablePostFileTask::oid() const
{
    return task_->oid();
}

void ReliablePostFileTask::cancel()
{
    // Must cancel itself first to set the canceled_ flag to avoid it being
    // retried
    FileServerTask::cancel();
    if (task_) {
        QMetaObject::invokeMethod(task_.data(), "cancel");
    }
}

void ReliablePostFileTask::onHttpRequestFinished()
{
    // Nothing to do. The actual POST http request is managed by PostFileTask
}

PostFileTask::PostFileTask(const QUrl& url,
                           const QString& parent_dir,
                           const QString& local_path,
                           QFile *file,
                           const QString& name,
                           const bool use_upload,
                           quint64 total_size,
                           quint64 start_offset,
                           quint32 chunk_size,
                           const bool need_idx_progress)
    : FileServerTask(url, local_path),
      parent_dir_(parent_dir),
      file_(file),
      name_(name),
      use_upload_(use_upload),
      total_size_(total_size),
      start_offset_(start_offset),
      chunk_size_(chunk_size),
      need_idx_progress_(need_idx_progress)
{
}

PostFileTask::PostFileTask(const QUrl& url,
                           const QString& parent_dir,
                           const QString& local_path,
                           QFile *file,
                           const QString& name,
                           const QString& relative_path,
                           quint64 total_size)
    : FileServerTask(url, local_path),
      parent_dir_(parent_dir),
      file_(file),
      name_(name),
      use_upload_(true),
      relative_path_(relative_path),
      total_size_(total_size),
      start_offset_(0),
      chunk_size_(-1)
{
}

PostFileTask::~PostFileTask()
{
}

void PostFileTask::prepare()
{
    // printThread("post file task is in thread");
}

bool PostFileTask::isChunked() const
{
    return chunk_size_ > 0;
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

    // "relative_path" param
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

    if (isChunked()) {
        // MMap the chunk to read the chunk content from the file
        uchar *buf = file_->map(start_offset_, chunk_size_);
        if (!buf) {
            setError(FileNetworkTask::FileIOError, "Failed to read file content");
            emit finished(false);
            return;
        }
        QBuffer *buffer = new QBuffer();
        buffer->setParent(this);
        // This would copy the mmaped file content to the QBuffer right now
        buffer->setData(QByteArray(reinterpret_cast<const char*>(buf), chunk_size_));
        buffer->open(QIODevice::ReadOnly);
        file_->unmap(buf);
        file_part.setBodyDevice(buffer);
    } else {
        file_part.setBodyDevice(file_);
    }
    multipart->append(file_part);

    // "need_idx_progress" param
    if (need_idx_progress_) {
        url_ = ::includeQueryParams(url_, {{"need_idx_progress", "true"}});
    }

    QNetworkRequest request(url_);
    request.setRawHeader("Content-Type",
                         "multipart/form-data; boundary=" + multipart->boundary());
    if (isChunked()) {
        setContentRangeHeader(&request);
    }

    reply_ = getQNAM()->post(request, multipart);

    connect(reply_, SIGNAL(sslErrors(const QList<QSslError>&)),
            this, SLOT(onSslErrors(const QList<QSslError>&)));
    connect(reply_, SIGNAL(finished()), this, SLOT(httpRequestFinished()));
    connect(reply_, SIGNAL(uploadProgress(qint64,qint64)),
            this, SIGNAL(progressUpdate(qint64, qint64)));
}

void PostFileTask::setContentRangeHeader(QNetworkRequest *request)
{
    auto range_header = QString("bytes %1-%2/%3")
                            .arg(QString::number(start_offset_),
                                 QString::number(start_offset_ + chunk_size_ - 1),
                                 QString::number(file_->size()));
    // printf("range_header = %s\n", range_header.toUtf8().data());
    request->setRawHeader("Content-Range", range_header.toUtf8());
    request->setRawHeader("Content-Disposition",
                          QString(kFileNameHeaderTemplate).arg(name_).toUtf8());
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
