#include <QUrl>
#include <QtNetwork>
#include <QSslError>
#include <QSslConfiguration>
#include <QSslCertificate>

#include "seafile-applet.h"
#include "certs-mgr.h"
#include "ui/main-window.h"
#include "ui/ssl-confirm-dialog.h"
#include "utils/utils.h"
#include "network-mgr.h"

#include "api-client.h"

namespace {

const char *kContentTypeForm = "application/x-www-form-urlencoded";
const char *kAuthHeader = "Authorization";

const int kMaxRedirects = 3;

bool shouldIgnoreRequestError(const QNetworkReply* reply)
{
    return reply->url().toString().contains("/api2/events");
}

} // namespace

QNetworkAccessManager* SeafileApiClient::na_mgr_ = NULL;

SeafileApiClient::SeafileApiClient(QObject *parent)
    : QObject(parent),
      reply_(NULL),
      redirect_count_(0)
{
    if (!na_mgr_) {
        static QNetworkAccessManager mNetworkAccessManager;
        na_mgr_ = &mNetworkAccessManager;
        NetworkManager::instance()->addWatch(na_mgr_);
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

    // qWarning("send request, url = %s\n, token = %s\n",
    //        request.url().toString().toUtf8().data(),
    //        request.rawHeader(kAuthHeader).data());

    reply_ = na_mgr_->get(request);

    connect(reply_, SIGNAL(sslErrors(const QList<QSslError>&)),
            this, SLOT(onSslErrors(const QList<QSslError>&)));

    connect(reply_, SIGNAL(finished()), this, SLOT(httpRequestFinished()));
}

void SeafileApiClient::post(const QUrl& url, const QByteArray& data, bool is_put)
{
    body_ = data;
    QNetworkRequest request(url);
    if (token_.length() > 0) {
        char buf[1024];
        qsnprintf(buf, sizeof(buf), "Token %s", token_.toUtf8().data());
        request.setRawHeader(kAuthHeader, buf);
    }
    request.setHeader(QNetworkRequest::ContentTypeHeader, kContentTypeForm);

    if (is_put)
        reply_ = na_mgr_->put(request, body_);
    else
        reply_ = na_mgr_->post(request, body_);

    connect(reply_, SIGNAL(finished()), this, SLOT(httpRequestFinished()));

    connect(reply_, SIGNAL(sslErrors(const QList<QSslError>&)),
            this, SLOT(onSslErrors(const QList<QSslError>&)));
}

void SeafileApiClient::deleteResource(const QUrl& url)
{
    QNetworkRequest request(url);

    if (token_.length() > 0) {
        char buf[1024];
        qsnprintf(buf, sizeof(buf), "Token %s", token_.toUtf8().data());
        request.setRawHeader(kAuthHeader, buf);
    }

    reply_ = na_mgr_->deleteResource(request);

    connect(reply_, SIGNAL(sslErrors(const QList<QSslError>&)),
            this, SLOT(onSslErrors(const QList<QSslError>&)));

    connect(reply_, SIGNAL(finished()), this, SLOT(httpRequestFinished()));
}

void SeafileApiClient::onSslErrors(const QList<QSslError>& errors)
{
    QUrl url = reply_->url();
    QSslCertificate cert = reply_->sslConfiguration().peerCertificate();

    if (cert.isNull()) {
        // The server has no ssl certificate, we do nothing and let the
        // request fail
        qWarning("the certificate for %s is null", url.toString().toUtf8().data());
        return;
    }

    CertsManager *mgr = seafApplet->certsManager();

    QSslCertificate saved_cert = mgr->getCertificate(url.toString());

    if (saved_cert.isNull()) {
        // dump certificate information
        qWarning() << "\n= SslErrors =\n" << dumpSslErrors(errors);
        qWarning() << "\n= Certificate =\n" << dumpCertificate(cert);

        // This is the first time when the client connects to the server.
        if (seafApplet->detailedYesOrNoBox(
            tr("<b>Warning:</b> The ssl certificate of this server is not trusted, proceed anyway?"),
            dumpSslErrors(errors) + dumpCertificate(cert), 0, false)) {
            mgr->saveCertificate(url, cert);
            reply_->ignoreSslErrors();
        }


        return;
    } else if (saved_cert == cert) {
        // The user has choosen to trust the certificate before
        reply_->ignoreSslErrors();
        return;
    } else {
        // dump certificate information
        qWarning() << "\n= SslErrors =\n" << dumpSslErrors(errors);
        qWarning() << "\n= Certificate =\n" << dumpCertificate(cert);
        qWarning() << "\n= Previous Certificate =\n" << dumpCertificate(saved_cert);

        /**
         * The cert which the user had chosen to trust has been changed. It
         * may be either:
         *
         * 1. The server has changed its ssl certificate
         * 2. The user's connection is under security attack
         *
         * Anyway, we'll prompt the user
         */
        SslConfirmDialog dialog(url,
                                dumpCertificateFingerprint(cert),
                                dumpCertificateFingerprint(saved_cert),
                                seafApplet->mainWindow());
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
        if (NetworkManager::instance()->shouldRetry(reply_->error())) {
            qWarning("[api] network proxy error, retrying\n");
            resendRequest(reply_->url());
            return;
        }
        if (!shouldIgnoreRequestError(reply_)) {
            qWarning("[api] network error for %s: %s\n", toCStr(reply_->url().toString()),
                   reply_->errorString().toUtf8().data());
        }
        emit networkError(reply_->error(), reply_->errorString());
        return;
    }

    if (handleHttpRedirect()) {
        return;
    }

    if ((code / 100) == 4 || (code / 100) == 5) {
        if (!shouldIgnoreRequestError(reply_)) {
            qWarning("request failed for %s: status code %d\n",
                   reply_->url().toString().toUtf8().data(), code);
        }
        emit requestFailed(code);
        return;
    }

    emit requestSuccess(*reply_);
}

bool SeafileApiClient::handleHttpRedirect()
{
    QVariant redirect_attr = reply_->attribute(QNetworkRequest::RedirectionTargetAttribute);
    if (redirect_attr.isNull()) {
        return false;
    }

    if (redirect_count_++ > kMaxRedirects) {
        // simply treat too many redirects as server side error
        emit requestFailed(500);
        qWarning("too many redirects for %s\n",
               reply_->url().toString().toUtf8().data());
        return true;
    }

    QUrl redirect_url = redirect_attr.toUrl();
    if (redirect_url.isRelative()) {
        redirect_url =  reply_->url().resolved(redirect_url);
    }

    // qWarning("redirect to %s (from %s)\n", redirect_url.toString().toUtf8().data(),
    //        reply_->url().toString().toUtf8().data());
    // don't handle with this, since rename operation returns a 301
    if (reply_->operation() == QNetworkAccessManager::PostOperation)
        return false;

    resendRequest(redirect_url);

    return true;
}

void SeafileApiClient::resendRequest(const QUrl& url)
{
    switch (reply_->operation()) {
    case QNetworkAccessManager::GetOperation:
        reply_->deleteLater();
        get(url);
        break;
    case QNetworkAccessManager::PostOperation:
        post(url, body_, false);
        break;
    case QNetworkAccessManager::PutOperation:
        post(url, body_, true);
        break;
    case QNetworkAccessManager::DeleteOperation:
        reply_->deleteLater();
        deleteResource(url);
        break;
    default:
        reply_->deleteLater();
        qWarning() << "unsupported operation" << reply_->operation()
          << "to" << url.toString()
          << "from" << reply_->url().toString();
        break;
    }
}
