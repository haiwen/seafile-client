#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QFile>
#include <QFileInfo>
#include <QByteArray>

#include "utils/utils.h"
#include "utils/file-utils.h"
#include "post-via-upload-link.h"

namespace {

const char *kParentDirParam = "form-data; name=\"parent_dir\"";
const char *kFileParamTemplate = "form-data; name=\"file\"; filename=\"%1\"";
const char *kContentTypeApplicationOctetStream = "application/octet-stream";

}

PostToUploadLink::PostToUploadLink(const QString url, const QString local_file)
    : FileServerTask(url, local_file), file_(NULL)
{
}

void PostToUploadLink::prepare()
{
    file_ = new QFile(local_path_);
    if (!file_->exists()) {
        qWarning("Upload %s failed. File does not exist.", toCStr(local_path_));
        return;
    }
    if (!file_->open(QIODevice::ReadOnly)) {
        qWarning("Upload %s failed. Couldn't open file.", toCStr(local_path_));
        return;
    }
}

void PostToUploadLink::sendRequest()
{
    QHttpMultiPart *multipart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    // parent_dir param
    QHttpPart parentdir_part;
    parentdir_part.setHeader(QNetworkRequest::ContentDispositionHeader,
                             kParentDirParam);
    parentdir_part.setBody(QString("/").toUtf8());
    multipart->append(parentdir_part);

    QHttpPart file_part;
    QString file_name = getBaseName(local_path_);
    file_part.setHeader(QNetworkRequest::ContentDispositionHeader,
                        QString(kFileParamTemplate).arg(file_name).toUtf8());
    file_part.setHeader(QNetworkRequest::ContentTypeHeader,
                        kContentTypeApplicationOctetStream);
    file_part.setBodyDevice(file_);
    multipart->append(file_part);

    QNetworkRequest request(url_);
    request.setRawHeader("Content-Type",
                         "multipart/form-data; boundary=" + multipart->boundary());

    reply_ = getQNAM()->post(request, multipart);
    connect(reply_, SIGNAL(finished()),
            this, SLOT(httpRequestFinished()));
    // connect(reply_, SIGNAL(uploadProgress(qint64, qint64)),
    //         this, SIGNAL(progressUpdate(qint64, qint64)));
}

void PostToUploadLink::onHttpRequestFinished()
{
    emit finished(true);
}
