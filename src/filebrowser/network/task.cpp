#include "task.h"

#include <QDebug>
#include <QDir>
#include <QTime>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QHttpMultiPart>
#include <QSslError>
#include "utils/utils.h"

#include "seafile-applet.h"
#include "certs-mgr.h"
#include <QSslConfiguration>
#include <QSslCertificate>

SeafileNetworkTask::SeafileNetworkTask(const QString &token,
                                       const QUrl &url,
                                       bool prefetch_api_required)
    :
    prefetch_api_url_buf_(prefetch_api_required ? new QByteArray : NULL),
    reply_(NULL),
    token_(token),
    url_(url),
    status_(prefetch_api_required ?
            SEAFILE_NETWORK_TASK_STATUS_FRESH :
            SEAFILE_NETWORK_TASK_STATUS_PREFETCHED)
{
    type_ = SEAFILE_NETWORK_TASK_UNKNOWN;
    network_mgr_ = new QNetworkAccessManager(this);
    req_ = new SeafileNetworkRequest(token_, url_);
#if !defined(QT_NO_OPENSSL)
    connect(network_mgr_, SIGNAL(sslErrors(QNetworkReply*, QList<QSslError>)),
             this, SLOT(sslErrors(QNetworkReply*, QList<QSslError>)));
#endif
    connect(this, SIGNAL(start()), this, SLOT(onStart()));
    connect(this, SIGNAL(cancel()), this, SLOT(onCancel()));
}
#if !defined(QT_NO_OPENSSL)
void SeafileNetworkTask::sslErrors(QNetworkReply*, const QList<QSslError> &errors)
{
    CertsManager *mgr = seafApplet->certsManager();

    QSslCertificate cert = reply_->sslConfiguration().peerCertificate();
    QSslCertificate saved_cert = mgr->getCertificate(url_.toString());
    if (!saved_cert.isNull() && saved_cert == cert)
        reply_->ignoreSslErrors();
    else {
        qWarning() << dumpSslErrors(errors);
        reply_->abort();
    }
}
#endif

SeafileNetworkTask::~SeafileNetworkTask()
{
    onClose();
    delete req_;
    delete prefetch_api_url_buf_;
}

void SeafileNetworkTask::onClose()
{
    if (reply_)
        onAborted();
    if (reply_)
        reply_->deleteLater();
}

void SeafileNetworkTask::onRedirected(const QUrl &new_url)
{
    qDebug() << "[network task]" << url_.toEncoded()
      << "is redirected to" << new_url.toEncoded();
    status_ = SEAFILE_NETWORK_TASK_STATUS_REDIRECTING;
    url_ = new_url;
    req_->setUrl(url_);
    reply_->deleteLater();
    reply_ = NULL;
}

void SeafileNetworkTask::onAborted(SeafileNetworkTaskError error)
{
    qDebug() << "[network task]" << url_.toEncoded()
      << " is aborted due to error" << error;
    status_ = SEAFILE_NETWORK_TASK_STATUS_ERROR;
    reply_->deleteLater();
    reply_ = NULL;
}

void SeafileNetworkTask::onStart()
{
    qDebug() << "[network task]" << url_.toEncoded() << "started";
    if (status_ == SEAFILE_NETWORK_TASK_STATUS_FRESH)
        startPrefetchRequest();
    else if (status_ == SEAFILE_NETWORK_TASK_STATUS_PREFETCHED)
        emit prefetchFinished();
}

void SeafileNetworkTask::onCancel()
{
    qDebug() << "[network task]" << url_.toEncoded() << "cancelled";
    if (reply_ &&
        status_ != SEAFILE_NETWORK_TASK_STATUS_ERROR &&
        status_ != SEAFILE_NETWORK_TASK_STATUS_CANCELING) {
        reply_->abort();
        status_ = SEAFILE_NETWORK_TASK_STATUS_CANCELING;
    }
}

void SeafileNetworkTask::startPrefetchRequest()
{
    if (status_ != SEAFILE_NETWORK_TASK_STATUS_FRESH &&
        status_ != SEAFILE_NETWORK_TASK_STATUS_REDIRECTING)
        return;
    status_ = SEAFILE_NETWORK_TASK_STATUS_PREFETCHING;
    reply_ = network_mgr_->get(*req_);
    connect(reply_, SIGNAL(finished()), this, SLOT(onPrefetchFinished()));
    connect(reply_, SIGNAL(readyRead()), this, SLOT(onPrefetchProcessReady()));

}

void SeafileNetworkTask::onPrefetchProcessReady()
{
    if (status_ == SEAFILE_NETWORK_TASK_STATUS_PREFETCHING)
        *prefetch_api_url_buf_ += reply_->readAll();
}

void SeafileNetworkTask::onPrefetchFinished()
{
    if (status_ == SEAFILE_NETWORK_TASK_STATUS_CANCELING) {
        emit prefetchAborted();
        return;
    }
    QVariant redirectionTarget = reply_->attribute(
        QNetworkRequest::RedirectionTargetAttribute);
    if (reply_->error()) {
        qDebug() << Q_FUNC_INFO << reply_->errorString();
        emit prefetchAborted();
        return;
    } else if (!redirectionTarget.isNull()) {
        QUrl new_url = QUrl(url_).resolved(redirectionTarget.toUrl());
        onRedirected(new_url);
        startPrefetchRequest();
        return;
    }
    QString new_url(*prefetch_api_url_buf_);
    if (new_url.size() <= 2) {
        emit prefetchAborted();
        return;
    }
    new_url.remove(0, 1);
    new_url.chop(1);
    if (reply_->hasRawHeader("oid"))
        emit prefetchOid(reply_->rawHeader("oid"));
    onRedirected(new_url);
    status_ = SEAFILE_NETWORK_TASK_STATUS_PREFETCHED;
    reply_->deleteLater();
    reply_ = NULL;
    emit prefetchFinished();
}

SeafileDownloadTask::SeafileDownloadTask(const QString &token,
                                         const QUrl &url,
                                         const QString &file_name,
                                         const QString &file_location,
                                         bool prefetch_api_required)
    :SeafileNetworkTask(token, url, prefetch_api_required),
    file_(NULL), file_name_(file_name), file_location_(file_location)
{
    type_ = SEAFILE_NETWORK_TASK_DOWNLOAD;
    connect(this, SIGNAL(prefetchAborted()), this, SLOT(onAborted()));
    //start download manually, invoke this after signal prefetchFinished
    connect(this, SIGNAL(resume()), this, SLOT(onStartDownload()));
}

SeafileDownloadTask::~SeafileDownloadTask()
{
    onClose();
    if (file_) {
        file_->close();
        delete file_;
        file_ = NULL;
    }
}

void SeafileDownloadTask::onClose()
{
    SeafileNetworkTask::onClose();
}

void SeafileDownloadTask::onStartDownload()
{
    file_ = new QFile(file_location_, this);
    if (file_->exists()) {
        qDebug() << "[download task]" << file_location_ << "exists";
        QFileInfo file_info(file_location_);
        QString alternative_file_location(file_info.dir().absolutePath() +
              "/" + file_info.baseName() +
              " (%1)." + file_info.completeSuffix());
        int i;
        for (i = 1; i != 10; i++) {
            file_->setFileName(alternative_file_location.arg(i));
            if (!file_->exists())
                break;
        }
        if (i == 10) {
            qDebug() << "[download task]" << file_location_
              << "unable to find an alternative file name";
            onAborted(SEAFILE_NETWORK_TASK_FILE_ERROR);
            return;
        }
        file_location_ = file_->fileName();
        qDebug() << "[download task]" << file_location_ << "is used";
        emit fileLocationChanged(file_location_);
    }
    if (!file_->open(QIODevice::WriteOnly)) {
        qDebug() << Q_FUNC_INFO << file_->errorString();
        onAborted(SEAFILE_NETWORK_TASK_FILE_ERROR);
        return;
    }

    emit started();
    startRequest();
}

void SeafileDownloadTask::startRequest()
{
    if (status_ != SEAFILE_NETWORK_TASK_STATUS_PREFETCHED &&
        status_ != SEAFILE_NETWORK_TASK_STATUS_REDIRECTING)
        return;
    status_ = SEAFILE_NETWORK_TASK_STATUS_PROCESSING;
    reply_ = network_mgr_->get(*req_);
    connect(reply_, SIGNAL(finished()), this, SLOT(httpFinished()));
    connect(reply_, SIGNAL(readyRead()), this, SLOT(httpProcessReady()));
    connect(reply_, SIGNAL(downloadProgress(qint64,qint64)),
            this, SLOT(httpUpdateProgress(qint64,qint64)));
}

void SeafileDownloadTask::httpUpdateProgress(qint64 processed_bytes, qint64 total_bytes)
{
    emit updateProgress(processed_bytes, total_bytes);
}

void SeafileDownloadTask::httpProcessReady()
{
    if (file_) {
        file_->write(reply_->readAll());
    }
}

void SeafileDownloadTask::httpFinished()
{
    if (status_ == SEAFILE_NETWORK_TASK_STATUS_CANCELING) {
        onAborted(SEAFILE_NETWORK_TASK_NO_ERROR);
        return;
    }
    file_->flush();
    file_->close();
    QVariant redirectionTarget = reply_->attribute(
        QNetworkRequest::RedirectionTargetAttribute);
    if (reply_->error()) {
        qDebug() << Q_FUNC_INFO << reply_->errorString();
        onAborted(SEAFILE_NETWORK_TASK_NETWORK_ERROR);
        return;
    } else if (!redirectionTarget.isNull()) {
        QUrl new_url = QUrl(url_).resolved(redirectionTarget.toUrl());
        onRedirected(new_url);
        return;
    }
    //onFinished
    qDebug() << "[download task]" << url_.toEncoded() << "finished";
    status_ = SEAFILE_NETWORK_TASK_STATUS_FINISHED;
    emit finished();
    this->deleteLater();
}

void SeafileDownloadTask::onRedirected(const QUrl &new_url)
{
    SeafileNetworkTask::onRedirected(new_url);
    file_->open(QIODevice::WriteOnly);
    file_->resize(0);
    emit redirected();
    startRequest();
}

void SeafileDownloadTask::onAborted(SeafileNetworkTaskError error)
{
    SeafileNetworkTask::onAborted(error);
    emit aborted();
    if (file_) {
        file_->close();
        file_->remove();
        delete file_;
        file_ = NULL;
    }
    this->deleteLater();
}

SeafileUploadTask::SeafileUploadTask(const QString &token,
                                         const QUrl &url,
                                         const QString &parent_dir,
                                         const QString &file_name,
                                         const QString &file_location,
                                         bool prefetch_api_required)
    :SeafileNetworkTask(token, url, prefetch_api_required),
    file_(NULL), parent_dir_(parent_dir),
    file_name_(file_name), file_location_(file_location),
    upload_parts_(NULL)
{
    type_ = SEAFILE_NETWORK_TASK_DOWNLOAD;
    connect(this, SIGNAL(prefetchAborted()), this, SLOT(onAborted()));
    connect(this, SIGNAL(resume()), this, SLOT(onStartUpload()));
}

SeafileUploadTask::~SeafileUploadTask()
{
    onClose();
    if (file_) {
        file_->close();
        delete file_;
        file_ = NULL;
    }
}

void SeafileUploadTask::onClose()
{
    SeafileNetworkTask::onClose();
}

void SeafileUploadTask::onStartUpload()
{
    file_ = new QFile(file_location_, this);
    upload_parts_ = new QHttpMultiPart(QHttpMultiPart::FormDataType, this);
    req_->setRawHeader("Content-Type", "multipart/form-data; boundary=" +
                                           upload_parts_->boundary());
    //file not exists
    if (!file_->exists()) {
        qDebug() << "[upload task]" << "file" << file_->fileName() << "not existed";
        onAborted(SEAFILE_NETWORK_TASK_FILE_ERROR);
        return;
    }
    //file is unable to open
    if (!file_->open(QIODevice::ReadOnly)) {
        qDebug() << "[upload task]" << "file" << file_->fileName() << "unable to open";
        qDebug() << Q_FUNC_INFO << file_->errorString();
        onAborted(SEAFILE_NETWORK_TASK_FILE_ERROR);
        return;
    }
    // all good to go
    QHttpPart text_part_;
    text_part_.setHeader(QNetworkRequest::ContentDispositionHeader,
                         QVariant("form-data; name=\"parent_dir\""));
    text_part_.setBody(parent_dir_.toUtf8().data());
    upload_parts_->append(text_part_);
    // all good to go for body
    QHttpPart upload_part_;
    upload_part_.setHeader(QNetworkRequest::ContentDispositionHeader,
        QVariant(QString("form-data; name=\"file\"; filename=\"%1\"")
                    .arg(file_name_)));
    upload_part_.setHeader(QNetworkRequest::ContentTypeHeader,
        QVariant("application/octet-stream"));
    upload_part_.setBodyDevice(file_);
    upload_parts_->append(upload_part_);

    emit started();
    startRequest();
}

void SeafileUploadTask::startRequest()
{
    if (status_ != SEAFILE_NETWORK_TASK_STATUS_PREFETCHED &&
        status_ != SEAFILE_NETWORK_TASK_STATUS_REDIRECTING)
        return;
    if (upload_parts_ == NULL)
        return;
    status_ = SEAFILE_NETWORK_TASK_STATUS_PROCESSING;
    reply_ = network_mgr_->post(*req_, upload_parts_);
    connect(reply_, SIGNAL(finished()), this, SLOT(httpFinished()));
    connect(reply_, SIGNAL(uploadProgress(qint64,qint64)),
            this, SLOT(httpUpdateProgress(qint64,qint64)));
}

void SeafileUploadTask::httpUpdateProgress(qint64 processed_bytes, qint64 total_bytes)
{
    emit updateProgress(processed_bytes, total_bytes);
}

void SeafileUploadTask::httpFinished()
{
    if (status_ == SEAFILE_NETWORK_TASK_STATUS_CANCELING) {
        onAborted(SEAFILE_NETWORK_TASK_NO_ERROR);
        return;
    }
    file_->close();
    QVariant redirectionTarget = reply_->attribute(
        QNetworkRequest::RedirectionTargetAttribute);
    if (reply_->error()) {
        qDebug() << Q_FUNC_INFO << reply_->errorString();
        onAborted(SEAFILE_NETWORK_TASK_NETWORK_ERROR);
        return;
    } else if (!redirectionTarget.isNull()) {
        QUrl new_url = QUrl(url_).resolved(redirectionTarget.toUrl());
        onRedirected(new_url);
        return;
    }
    //onFinished
    qDebug() << "[upload task]" << url_.toEncoded() << "finished";
    status_ = SEAFILE_NETWORK_TASK_STATUS_FINISHED;
    emit finished();
    this->deleteLater();
}

void SeafileUploadTask::onRedirected(const QUrl &new_url)
{
    SeafileNetworkTask::onRedirected(new_url);
    emit redirected();
    file_->seek(0);
    startRequest();
}

void SeafileUploadTask::onAborted(SeafileNetworkTaskError error)
{
    SeafileNetworkTask::onAborted(error);
    emit aborted();
    this->deleteLater();
}

