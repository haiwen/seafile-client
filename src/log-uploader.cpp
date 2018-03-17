
#include <QtGlobal>

#include <QSysInfo>

#include "utils/utils.h"
#include "utils/file-utils.h"
#include "post-via-upload-link.h"
#include "seafile-applet.h"
#include "configurator.h"
#include "account-mgr.h"
#include "filebrowser/progress-dialog.h"

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

void LogDirUploader::run() {
    QString log_path = QDir(seafApplet->configurator()->ccnetDir()).absoluteFilePath("logs");
    QString username = seafApplet->accountManager()->currentAccount().username;
    QString time = QDateTime::currentDateTime().toString("yyyy.MM.dd-hh.mm.ss");
    compressed_file_name_ = log_path + '-' + time + '-' + username;

    if (!JlCompress::compressDir(compressed_file_name_, log_path)) {
        qWarning("create log archive failed");
        emit finished(false);
    } else {
        SeafileApiRequest * get_upload_link_req = new GetUploadFileLinkRequest(getLogUploadLink());
        connect(get_upload_link_req, SIGNAL(success(const QString&)),
                this, SLOT(onGetUploadLinkSuccess(const QString&)));
        connect(get_upload_link_req, SIGNAL(failed(const ApiError&)),
                this, SLOT(onGetUploadLinkFailed(const ApiError&)));
        get_upload_link_req->send();
    }
}

void LogDirUploader::onGetUploadLinkSuccess(const QString& upload_link)
{
    PostToUploadLink *uploader = new PostToUploadLink(
        upload_link, compressed_file_name_);
    connect(uploader, SIGNAL(finished(bool)),
            this, SLOT(onUploadLogDirFinished(bool)));
    uploader->start();

    progress_dlg_ = new QProgressDialog();
    connect(uploader->getReply(), SIGNAL(progressUpdate(qint64, qint64)),
            this, SLOT(onProgressUpdate(qint64, qint64)));

    // set dialog attributes
    progress_dlg_->setAttribute(Qt::WA_DeleteOnClose);
    progress_dlg_->setWindowModality(Qt::NonModal);
    progress_dlg_->setWindowTitle(tr("Upload log files"));
    progress_dlg_->show();
    progress_dlg_->raise();
    progress_dlg_->activateWindow();

}

void LogDirUploader::onProgressUpdate(qint64 processed_bytes, qint64 total_bytes)
{
    if (total_bytes > INT_MAX) {
        if (progress_dlg_->maximum() != INT_MAX) {
            progress_dlg_->setMaximum(INT_MAX);
        }

        // Avoid overflow
        double progress = double(processed_bytes) * INT_MAX / total_bytes;
        progress_dlg_->setValue((int)progress);
    } else {
        if (progress_dlg_->maximum() != total_bytes) {
            progress_dlg_->setMaximum(total_bytes);
        }

        progress_dlg_->setValue(processed_bytes);
    }

    progress_dlg_->setLabelText(tr("%1 of %2")
        .arg(::readableFileSizeV2(processed_bytes))
        .arg(::readableFileSizeV2(total_bytes)));
}

void LogDirUploader::onGetUploadLinkFailed(const ApiError& error)
{
    emit finished(false);
}

void LogDirUploader::onUploadLogDirFinished(bool success)
{
    if (!success) {
        emit finished(false);
    } else {
        if (QFile::exists(compressed_file_name_)) {
            QFile::remove(compressed_file_name_);
        }
        emit finished(true);
    }
}
