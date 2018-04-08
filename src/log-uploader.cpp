
#include <QtGlobal>

#include <QSysInfo>
#include <QThreadPool>

#include "utils/utils.h"
#include "utils/file-utils.h"
#include "seafile-applet.h"
#include "configurator.h"
#include "account-mgr.h"

#include "log-uploader.h"
#include "JlCompress.h"
#include "api/requests.h"

namespace {

const char* kGetUploadFileLink = "http://192.168.1.113:8000/api/v2.1/upload-links/dcfa5dd6645e4f38b823/upload/";
const QString getLogUploadLink()
{
    QString upload_link = qgetenv("SEAFILE_LOG_UPLOAD_LINK");
    return !upload_link.isEmpty() ? upload_link : kGetUploadFileLink;
}

} // namespace


LogDirUploader::LogDirUploader()
    : task_(nullptr)
{
}

LogDirUploader::~LogDirUploader()
{
    if (task_) {
        task_->deleteLater();
        task_ = nullptr;
    }
}

void LogDirUploader::start()
{
    progress_dlg_ = new LogUploadProgressDialog();
    connect(progress_dlg_, SIGNAL(canceled()),
            this, SLOT(onCanceled()));

    LogDirCompressor *uploader = new LogDirCompressor;
    connect(uploader, SIGNAL(compressFinished(bool, const QString&)),
            this, SLOT(onCompressFinished(bool, const QString&)));
    uploader->setAutoDelete(true);
    QThreadPool::globalInstance()->start(uploader);
}

void LogDirUploader::onFinished()
{
    deleteLater();
    if (progress_dlg_) {
        progress_dlg_->accept();
        progress_dlg_ = nullptr;
    }
}

void LogDirUploader::onCanceled()
{
    if (task_) {
        task_->cancel();
    }
    progress_dlg_ = nullptr;
    if (QFile::exists(compressed_file_name_)) {
        QFile::remove(compressed_file_name_);
    }
    onFinished();
    emit finished();
}

void LogDirUploader::onCompressFinished(bool success, const QString& compressed_file_name)
{
    compressed_file_name_ = compressed_file_name;
    if (success) {
        SeafileApiRequest * get_upload_link_req = new GetUploadFileLinkRequest(getLogUploadLink());
        connect(get_upload_link_req, SIGNAL(success(const QString&)),
                this, SLOT(onGetUploadLinkSuccess(const QString&)));
        connect(get_upload_link_req, SIGNAL(failed(const ApiError&)),
                this, SLOT(onGetUploadLinkFailed(const ApiError&)));
        get_upload_link_req->send();
    } else {
        seafApplet->warningBox(tr("Upload log files failed"));
        onFinished();
    }
}

void LogDirUploader::onGetUploadLinkSuccess(const QString& upload_link)
{
    if (progress_dlg_ == nullptr) {
        if (QFile::exists(compressed_file_name_)) {
            QFile::remove(compressed_file_name_);
        }
        return;
    }
    progress_dlg_->setWindowTitle(tr("Upload log files"));
    QFile *file = new QFile(compressed_file_name_);
    if (!file->exists()) {
        qWarning("Upload %s failed. File does not exist.", toCStr(compressed_file_name_));
        return;
    }
    if (!file->open(QIODevice::ReadOnly)) {
        qWarning("Upload %s failed. Couldn't open file.", toCStr(compressed_file_name_));
        return;
    }
    QString file_name = getBaseName(compressed_file_name_);
    task_ = new PostFileTask(QUrl(upload_link), QString("/"),
                                           compressed_file_name_, file, file_name, QString(), 0);
    connect(task_, SIGNAL(finished(bool)), this, SLOT(onUploadLogDirFinished(bool)));
    connect(task_, SIGNAL(progressUpdate(qint64, qint64)),
            this, SLOT(onProgressUpdate(qint64, qint64)));

    task_->start();
}

void LogDirUploader::onGetUploadLinkFailed(const ApiError &)
{
    qWarning("Get upload link failed.");
    seafApplet->warningBox(tr("Upload log files failed"));
    onFinished();
}

void LogDirUploader::onProgressUpdate(qint64 processed_bytes, qint64 total_bytes)
{
    if (total_bytes == 0) {
        return;
    }
    if (task_->canceled()) {
        return;
    }
    progress_dlg_->updateData(processed_bytes, total_bytes);
}

void LogDirUploader::onUploadLogDirFinished(bool success)
{
    if (task_->canceled()) {
        return;
    }

    if (progress_dlg_) {
        progress_dlg_ = nullptr;
    }

    if (!success) {
        QString _error;
        if (task_->httpErrorCode() == 403) {
            _error = tr("Permission Error!");
        } else if (task_->httpErrorCode() == 404) {
            _error = tr("Library/Folder not found.");
        } else if (task_->httpErrorCode() == 401) {
            _error = tr("Authorization expired");
        } else {
            _error = task_->errorString();
        }
        QString msg = tr("Upload log files failed :%1").arg(_error);
        seafApplet->warningBox(msg);
    } else {

        if (QFile::exists(compressed_file_name_)) {
            QFile::remove(compressed_file_name_);
        }
        seafApplet->messageBox(tr("Successfully uploaded log files"));
    }
    onFinished();
    emit finished();
}

LogUploadProgressDialog::LogUploadProgressDialog(QWidget *parent)
        : QProgressDialog(parent)
{
    // set dialog attributes
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowModality(Qt::NonModal);
    setWindowTitle(tr("Compressing"));
    setCancelButtonText(tr("Cancel"));
    show();
    raise();
    activateWindow();
}

LogUploadProgressDialog::~LogUploadProgressDialog()
{
}

void LogUploadProgressDialog::updateData(qint64 processed_bytes, qint64 total_bytes)
{
    if (total_bytes > INT_MAX) {
        if (maximum() != INT_MAX) {
            setMaximum(INT_MAX);
        }

        // Avoid overflow
        double progress = double(processed_bytes) * INT_MAX / total_bytes;
        setValue((int)progress);
    } else {
        if (maximum() != total_bytes) {
            setMaximum(total_bytes);
        }

        setValue(processed_bytes);
    }

    setLabelText(tr("%1 of %2")
        .arg(::readableFileSizeV2(processed_bytes))
        .arg(::readableFileSizeV2(total_bytes)));
}

void LogDirCompressor::run() {
    QString log_path = QDir(seafApplet->configurator()->ccnetDir()).absoluteFilePath("logs");
    QString username = seafApplet->accountManager()->currentAccount().username;
    QString time = QDateTime::currentDateTime().toString("yyyy.MM.dd-hh.mm.ss");
    compressed_file_name_ = log_path + '-' + time + '-' + username + ".zip";

    if (!JlCompress::compressDir(compressed_file_name_, log_path)) {
        qWarning("create log archive failed");
        emit compressFinished(false);
    } else {
        emit compressFinished(true, compressed_file_name_);
    }
}
