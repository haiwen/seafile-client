#include <QUrl>
#include <QtNetwork>
#include <QSslError>
#include <QSslConfiguration>
#include <QSslCertificate>

#include "seafile-applet.h"
#include "customization-service.h"
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
const int kMaxHttpErrorLogLen = 300;

bool shouldIgnoreRequestError(const QNetworkReply* reply)
{
    return reply->url().toString().contains("/api2/events");
}

QString getQueryValue(const QUrl& url, const QString& name)
{
    QString v;
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    v = QUrlQuery(url.query()).queryItemValue(name);
#else
    v = url.queryItemValue(name);
#endif
    return QUrl::fromPercentEncoding(v.toUtf8());
}

} // namespace

QNetworkAccessManager* SeafileApiClient::na_mgr_ = NULL;

SeafileApiClient::SeafileApiClient(QObject *parent)
    : QObject(parent),
      reply_(NULL),
      redirect_count_(0),
      use_cache_(false)
{
    if (!na_mgr_) {
        static QNetworkAccessManager *manager = new QNetworkAccessManager(qApp);
        na_mgr_ = manager;
        NetworkManager::instance()->addWatch(na_mgr_);
        na_mgr_->setCache(CustomizationService::instance()->diskCache());
    }
}

SeafileApiClient::~SeafileApiClient()
{
    if (reply_) {
        reply_->deleteLater();
    }
}

void SeafileApiClient::prepareRequest(QNetworkRequest *req)
{
    if (use_cache_) {
        req->setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferNetwork);
        req->setAttribute(QNetworkRequest::CacheSaveControlAttribute, true);
    } else {
        req->setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
        req->setAttribute(QNetworkRequest::CacheSaveControlAttribute, false);
    }
}

void SeafileApiClient::get(const QUrl& url)
{
    QNetworkRequest request(url);
    prepareRequest(&request);

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
    prepareRequest(&request);

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
    prepareRequest(&request);

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
    const QUrl url = reply_->url();
    CertsManager *mgr = seafApplet->certsManager();

    // a copy of root CA
    QSslCertificate cert;

    Q_FOREACH (const QSslError &error, errors)
    {
        if (error.error() == QSslError::SelfSignedCertificate) {
            cert = error.certificate();
        }
    }

    // if we have other kinds of ssl errors, then fallback to peerCertificateChain
    if (cert.isNull()) {
        cert = reply_->sslConfiguration().peerCertificateChain().back();
    }

    // no ssl certificate?
    if (cert.isNull()) {
        qWarning("the certificate for %s is null", url.toString().toUtf8().data());
        reply_->abort();
        return;
    }
    QSslCertificate saved_cert = mgr->getCertificate(url.toString());

    //
    // The user has choosen to trust this root CA before
    // it is safe to ignore
    //
    if (saved_cert == cert) {
        reply_->ignoreSslErrors();
        return;
    }

    qWarning() << "\n= SslError =\n" << errors;
    qWarning() << dumpCipher(reply_->sslConfiguration().sessionCipher());
    qWarning() << dumpCertificate(cert);
    if (!saved_cert.isNull())
        qWarning() << dumpCertificate(saved_cert);

    //
    // The cert which the user had chosen to trust has been changed. It
    // may be either:
    //
    // 1. The server has changed its ssl certificate
    // 2. The user's connection is under security attack
    //
    // Anyway, we'll prompt the user
    //
    if (!saved_cert.isNull()) {
        SslConfirmDialog dialog(url, cert, saved_cert, seafApplet->mainWindow());
        if (dialog.exec() == QDialog::Accepted) {
            if (dialog.rememberChoice()) {
                mgr->saveCertificate(url, cert);
            }
            reply_->ignoreSslErrors();
            return;
        }
        reply_->abort();
        return;
    }

    //
    // This is the first time when the client connects to the server.
    //
    if (seafApplet->detailedYesOrNoBox(tr("<b>Warning:</b> %1, proceed anyway?")
                                           .arg(errors.front().errorString()),
                                       errors.front().errorString() + "\n" +
                                           dumpCertificate(cert),
                                       0, false)) {
        mgr->saveCertificate(url, cert);
        reply_->ignoreSslErrors();
        return;
    }

    // no process? abort
    reply_->abort();
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
            QByteArray content = reply_->readAll();
            qWarning("request failed for %s: %s\n",
                     reply_->url().toString().toUtf8().data(),
                     content.left(kMaxHttpErrorLogLen).data());
            if (content.length() > kMaxHttpErrorLogLen) {
                qDebug("request failed for %s: %s\n",
                       reply_->url().toString().toUtf8().data(),
                       content.data());
            }
        }
        emit requestFailed(code);
        return;
    }

    emit requestSuccess(*reply_);
}

// Return true if the request is redirected and request is resended.
bool SeafileApiClient::handleHttpRedirect()
{
    QVariant redirect_attr = reply_->attribute(QNetworkRequest::RedirectionTargetAttribute);
    if (redirect_attr.isNull()) {
        return false;
    }

    QUrl redirect_url = redirect_attr.toUrl();
    if (redirect_url.isRelative()) {
        redirect_url =  reply_->url().resolved(redirect_url);
    }

    // printf("redirect to %s (from %s)\n", redirect_url.toString().toUtf8().data(),
    //        reply_->url().toString().toUtf8().data());
    if (reply_->operation() == QNetworkAccessManager::PostOperation) {
        // XXX: Special case for rename/move file api, which returns 301 on
        // success. We need to distinguish that from a normal 301 redirect.
        // (In contrast, Rename/move dir api returns 200 on success).
        if (redirect_url.path().contains(QRegExp("/api2/repos/[^/]+/file/"))) {
            QString old_name = getQueryValue(reply_->url(), "p");
            QString new_name = getQueryValue(redirect_url, "p");
            // Only treat it as a rename file success when old and new are different
            if (!old_name.isEmpty() && !new_name.isEmpty() && old_name != new_name) {
                // printf ("get 301 rename file success, old_name: %s, new_name: %s\n",
                //         toCStr(old_name), toCStr(new_name));
                return false;
            }
        }
    }


    if (redirect_count_++ > kMaxRedirects) {
        // simply treat too many redirects as server side error
        emit requestFailed(500);
        qWarning("too many redirects for %s\n",
               reply_->url().toString().toUtf8().data());
        return true;
    }

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
