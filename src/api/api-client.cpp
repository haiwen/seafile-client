#include <QUrl>
#include <QtNetwork>
#include <QSslError>
#include <QSslConfiguration>
#include <QSslCertificate>

#include "seafile-applet.h"
#include "certs-mgr.h"
#include "ui/main-window.h"
#include "ui/ssl-confirm-dialog.h"

#include "api-client.h"

namespace {

const char *kContentTypeForm = "application/x-www-form-urlencoded";
const char *kAuthHeader = "Authorization";

} // namespace

QNetworkAccessManager* SeafileApiClient::na_mgr_ = NULL;

SeafileApiClient::SeafileApiClient()
{
    if (!na_mgr_) {
        na_mgr_ = new QNetworkAccessManager();
    }
}

SeafileApiClient::~SeafileApiClient()
{
    if (reply_) {
        reply_->deleteLater();
    }
}

void SeafileApiClient::get(const QUrl& url)
{
    QNetworkRequest request(url);

    if (token_.length() > 0) {
        char buf[1024];
        qsnprintf(buf, sizeof(buf), "Token %s", token_.toUtf8().data());
        request.setRawHeader(kAuthHeader, buf);
    }

    // qDebug("send request, url = %s\n, token = %s\n",
    //        request.url().toString().toUtf8().data(),
    //        request.rawHeader(kAuthHeader).data());

    reply_ = na_mgr_->get(request);

    connect(reply_, SIGNAL(sslErrors(const QList<QSslError>&)),
            this, SLOT(onSslErrors(const QList<QSslError>&)));

    connect(reply_, SIGNAL(finished()), this, SLOT(httpRequestFinished()));
}

void SeafileApiClient::post(const QUrl& url, const QByteArray& encodedParams)
{
    QNetworkRequest request(url);
    if (token_.length() > 0) {
        char buf[1024];
        qsnprintf(buf, sizeof(buf), "Token %s", token_.toUtf8().data());
        request.setRawHeader(kAuthHeader, buf);
    }
    request.setHeader(QNetworkRequest::ContentTypeHeader, kContentTypeForm);

    reply_ = na_mgr_->post(request, encodedParams);

    connect(reply_, SIGNAL(finished()), this, SLOT(httpRequestFinished()));

    connect(reply_, SIGNAL(sslErrors(const QList<QSslError>&)),
            this, SLOT(onSslErrors(const QList<QSslError>&)));
}

void SeafileApiClient::onSslErrors(const QList<QSslError>& errors)
{
    QUrl url = reply_->url();
    QSslCertificate cert = reply_->sslConfiguration().peerCertificate();
    if (cert.isNull()) {
        // The server has no ssl certificate, we do nothing and let the
        // request fail
        qDebug("the certificate for %s is null", url.toString().toUtf8().data());
        return;
    }

    CertsManager *mgr = seafApplet->certsManager();

    QSslCertificate saved_cert = mgr->getCertificate(url.toString());
    if (saved_cert.isNull()) {
        // This is the first time when the client connects to the server.
        QString question = tr("<b>Warning:</b> The ssl certificate of this server is not trusted, proceed anyway?");
        if (seafApplet->yesOrNoBox(question)) {
            mgr->saveCertificate(url, cert);
            reply_->ignoreSslErrors();
        }

        return;
    } else if (saved_cert == cert) {
        // The user has choosen to trust the certificate before
        reply_->ignoreSslErrors();
        return;
    } else {
        /**
         * The cert which the user had chosen to trust has been changed. It
         * may be either:
         *
         * 1. The server has changed its ssl certificate
         * 2. The user's connection is under security attack
         *
         * Anyway, we'll prompt the user
         */

        SslConfirmDialog dialog(url, seafApplet->mainWindow());
        if (dialog.exec() == QDialog::Accepted) {
            reply_->ignoreSslErrors();
            if (dialog.rememberChoice()) {
                mgr->saveCertificate(url, cert);
            }
        } else {
            reply_->abort();
        }
        return;
    }

    // SslConfirmDialog *dialog = new SslConfirmDialog(url, cert, errors, seafApplet->mainWindow());
    // dialog->show();
    // dialog->raise();
    // dialog->activateWindow();
}

void SeafileApiClient::httpRequestFinished()
{
    int code = reply_->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (code == 0 && reply_->error() != QNetworkReply::NoError) {
        qDebug("[api] network error: %s\n", reply_->errorString().toUtf8().data());
        emit networkError(reply_->error(), reply_->errorString());
        return;
    }

    if ((code / 100) == 4 || (code / 100) == 5) {
        qDebug("request failed : status code %d\n", code);
        emit requestFailed(code);
        return;
    }

    emit requestSuccess(*reply_);
}
