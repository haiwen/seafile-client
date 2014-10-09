#include "task.h"

#include <QDebug>
#include <QDir>
#include <QTime>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QHttpMultiPart>
#include <QUuid>
#include <QSslError>
#include "utils/utils.h"

#include "seafile-applet.h"
#include "certs-mgr.h"
#include <QSslConfiguration>
#include <QSslCertificate>

SeafileNetworkTask::SeafileNetworkTask(const QString &token,
                                       const QUrl &url,
                                       const QString &file_name,
                                       const QString &file_location)
    : reply_(NULL),
    file_(NULL), file_name_(file_name), file_location_(file_location),
    token_(token), url_(url),
    status_(SEAFILE_NETWORK_TASK_STATUS_FRESH)
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
    try {
        close();
    } catch (...) {
        qWarning() << "[network task]" <<
          Q_FUNC_INFO << "caucht an exception";
    }
}

void SeafileNetworkTask::close()
{
    if (reply_ && !reply_->isFinished())
        reply_->abort();
    if (reply_)
        reply_->deleteLater(); // delete called only here and onRedirected
    if (file_) {
        file_->close();
        file_->deleteLater();
    }
    delete req_;
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
}

void SeafileNetworkTask::onStart()
{
    this->resume(); //call virtual function to start task
}

void SeafileNetworkTask::onCancel()
{
    qDebug() << "[network task]" << url_.toEncoded() << "cancelled";
    if (reply_ &&
        !reply_->isFinished() &&
        status_ != SEAFILE_NETWORK_TASK_STATUS_ERROR &&
        status_ != SEAFILE_NETWORK_TASK_STATUS_CANCELING) {
        reply_->abort();
        status_ = SEAFILE_NETWORK_TASK_STATUS_CANCELING;
    }
}

SeafileDownloadTask::SeafileDownloadTask(const QString &token,
                                         const QUrl &url,
                                         const QString &file_name,
                                         const QString &file_location)
    :SeafileNetworkTask(token, url, file_name, file_location)
{
    type_ = SEAFILE_NETWORK_TASK_DOWNLOAD;
    //start download manually, invoke this after signal start
}

SeafileDownloadTask::~SeafileDownloadTask()
{
}

void SeafileDownloadTask::resume()
{
    qDebug() << "[download task]" << url_.toEncoded() << "started";

    // create QFile object, used for tmp file
    file_ = new QFile;

    // preparation of attempt to find an alternative file name
    QDir dir = QFileInfo(file_location_).absoluteDir();
    int count = 0;
    QString tmp_name;

    // attempt to find an alternative file name
    do {
        // if try too many times
        if (count++ > 10) {
            qDebug() << "[download task]" <<
              "file" << file_->fileName() << "unable to find a alternative file name";
            onAborted(SEAFILE_NETWORK_TASK_FILE_ERROR);
            return;
        }
        tmp_name = ::md5(QUuid::createUuid());
        file_->setFileName(dir.absoluteFilePath("." + tmp_name));
    } while(file_->exists());

    // open the file
    if (!file_->open(QIODevice::WriteOnly)) {
        qDebug() << "[download task]" <<
          "file" << file_->fileName() << "unable to write data";
        qDebug() << Q_FUNC_INFO << file_->errorString();
        onAborted(SEAFILE_NETWORK_TASK_FILE_ERROR);
        return;
    }

    emit started();
    startRequest();
}

void SeafileDownloadTask::startRequest()
{
    if (status_ != SEAFILE_NETWORK_TASK_STATUS_FRESH &&
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
    Q_ASSERT(file_ && "file_ must not be NULL");
    if (file_->write(reply_->readAll()) < 0) {
        qDebug() << "[download task]" << "file" << file_->fileName() << "unable to write data";
        qDebug() << Q_FUNC_INFO << file_->errorString();
        onAborted(SEAFILE_NETWORK_TASK_FILE_ERROR);
        return;
    }
}

void SeafileDownloadTask::httpFinished()
{
    if (status_ == SEAFILE_NETWORK_TASK_STATUS_CANCELING) {
        onAborted(SEAFILE_NETWORK_TASK_NO_ERROR);
        return;
    }

    // error handling and prepare for redirect
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

    QFile target_file_(file_location_);

    // remove if exist
    if (target_file_.exists() && !target_file_.remove()) {
        qDebug() << "[download task]" <<
            "file" << target_file_.fileName() << "unable to remove old file";
        qDebug() << Q_FUNC_INFO << target_file_.errorString();
        onAborted(SEAFILE_NETWORK_TASK_FILE_ERROR);
        return;
    }

    // rename the tmp file to the target location
    if (!file_->rename(file_location_)) {
        qDebug() << "[download task]" <<
            "file" << file_->fileName() << "unable to rename";
        qDebug() << Q_FUNC_INFO << file_->errorString();
        onAborted(SEAFILE_NETWORK_TASK_FILE_ERROR);
        return;
    }

    status_ = SEAFILE_NETWORK_TASK_STATUS_FINISHED;
    emit finished();
    this->deleteLater();
}

void SeafileDownloadTask::onRedirected(const QUrl &new_url)
{
    SeafileNetworkTask::onRedirected(new_url);
    if (!file_->open(QIODevice::WriteOnly) || !file_->resize(0)) {
        qDebug() << "[download task]" <<
          "file" << file_->fileName() << "unable to be truncated";
        qDebug() << Q_FUNC_INFO << file_->errorString();
        onAborted(SEAFILE_NETWORK_TASK_FILE_ERROR);
        return;
    }
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
    }
    this->deleteLater();
}

SeafileUploadTask::SeafileUploadTask(const QString &token,
                                     const QUrl &url,
                                     const QString &parent_dir,
                                     const QString &file_name,
                                     const QString &file_location)
    :SeafileNetworkTask(token, url, file_name, file_location),
    parent_dir_(parent_dir), upload_parts_(NULL)
{
    type_ = SEAFILE_NETWORK_TASK_DOWNLOAD;
}

SeafileUploadTask::~SeafileUploadTask()
{
}

void SeafileUploadTask::resume()
{
    qDebug() << "[upload task]" << url_.toEncoded() << "started";

    file_ = new QFile(file_location_);
    upload_parts_ = new QHttpMultiPart(QHttpMultiPart::FormDataType, this);
    req_->setRawHeader("Content-Type", "multipart/form-data; boundary=" +
                                           upload_parts_->boundary());
    //file not exists
    if (!file_->exists()) {
        qDebug() << "[upload task]" <<
          "file" << file_->fileName() << "not existed";
        onAborted(SEAFILE_NETWORK_TASK_FILE_ERROR);
        return;
    }

    //file is unable to open
    if (!file_->open(QIODevice::ReadOnly)) {
        qDebug() << "[upload task]" <<
          "file" << file_->fileName() << "unable to read data";
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
    if (status_ != SEAFILE_NETWORK_TASK_STATUS_FRESH &&
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

    // error handling and prepare for redirect
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

