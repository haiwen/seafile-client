#include <QUrl>
#include <QtNetwork>
#include <QSslError>
#include <QSslConfiguration>
#include <QSslCertificate>
#include <QNetworkConfigurationManager>

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
const char *kSeafileClientVersionHeader = "X-Seafile-Client-Version";

const int kMaxRedirects = 3;
const int kMaxHttpErrorLogLen = 300;

bool shouldIgnoreRequestError(const QNetworkReply* reply)
{
    return reply->url().toString().contains("/api2/events");
}

QString getQueryValue(const QUrl& url, const QString& name)
{
    QString v;
    v = QUrlQuery(url.query()).queryItemValue(name);
    return QUrl::fromPercentEncoding(v.toUtf8());
}

QNetworkAccessManager *createQNAM() {
    QNetworkAccessManager *manager = new QNetworkAccessManager(qApp);
    NetworkManager::instance()->addWatch(manager);
    manager->setCache(CustomizationService::instance()->diskCache());

    // From: http://www.qtcentre.org/threads/37514-use-of-QNetworkAccessManager-networkAccessible
    //
    // QNetworkAccessManager::networkAccessible is not explicitly set when the
    // QNetworkAccessManager is created. It is only set after the network
    // session is initialized. The session is initialized automatically when you
    // make a network request or you can initialize it before hand with
    // QNetworkAccessManager::setConfiguration() or the
    // QNetworkConfigurationManager::NetworkSessionRequired flag is set.
    manager->setConfiguration(
        QNetworkConfigurationManager().defaultConfiguration());
    return manager;
}

} // namespace

QNetworkAccessManager* SeafileApiClient::qnam_ = nullptr;
void SeafileApiClient::resetQNAM()
{
    if (qnam_) {
        qnam_->deleteLater();
    }
    qnam_ = createQNAM();
}

SeafileApiClient::SeafileApiClient(QObject *parent)
    : QObject(parent),
      reply_(NULL),
      redirect_count_(0),
      use_cache_(false)
{
    if (!qnam_) {
        qnam_ = createQNAM();
    }
    connect(qnam_, SIGNAL(destroyed(QObject *)),
            this, SLOT(doAbort()));
}

void SeafileApiClient::doAbort()
{
    if (reply_ && reply_->isRunning()) {
        qWarning("aborting api request %s on network error", toCStr(reply_->url().toString()));
        reply_->abort();
    }
}

SeafileApiClient::~SeafileApiClient()
{
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
    if (token_.length() > 0) {
        char buf[1024];
        qsnprintf(buf, sizeof(buf), "Token %s", token_.toUtf8().data());
        req->setRawHeader(kAuthHeader, buf);
    }

    foreach (const QString& key, headers_.keys()) {
        req->setRawHeader(key.toUtf8().data(), headers_[key].toUtf8().data());
    }

    req->setRawHeader(kSeafileClientVersionHeader, STRINGIZE(SEAFILE_CLIENT_VERSION));
}

void SeafileApiClient::get(const QUrl& url)
{
    QNetworkRequest request(url);
    prepareRequest(&request);

    reply_ = qnam_->get(request);
    // By default the parent object of the reply instance would be the
    // QNetworkAccessManager, and we delete the reply in our destructor. But now
    // we may recreate the QNetworkAccessManager when the connection status is
    // changed, which means the reply may already have been deleted when the
    // destructor is called. To avoid this, we explicitly set the
    // SeafileApiClient as the parent object of the reply.
    reply_->setParent(this);

    connect(reply_, SIGNAL(sslErrors(const QList<QSslError>&)),
            this, SLOT(onSslErrors(const QList<QSslError>&)));

    connect(reply_, SIGNAL(finished()), this, SLOT(httpRequestFinished()));
}

void SeafileApiClient::post(const QUrl& url, const QByteArray& data, bool is_put)
{
    body_ = data;
    QNetworkRequest request(url);
    prepareRequest(&request);

    request.setHeader(QNetworkRequest::ContentTypeHeader, kContentTypeForm);

    if (is_put)
        reply_ = qnam_->put(request, body_);
    else
        reply_ = qnam_->post(request, body_);

    reply_->setParent(this);

    connect(reply_, SIGNAL(finished()), this, SLOT(httpRequestFinished()));

    connect(reply_, SIGNAL(sslErrors(const QList<QSslError>&)),
            this, SLOT(onSslErrors(const QList<QSslError>&)));
}

void SeafileApiClient::deleteResource(const QUrl& url)
{
    QNetworkRequest request(url);
    prepareRequest(&request);

    reply_ = qnam_->deleteResource(request);
    reply_->setParent(this);

    connect(reply_, SIGNAL(sslErrors(const QList<QSslError>&)),
            this, SLOT(onSslErrors(const QList<QSslError>&)));

    connect(reply_, SIGNAL(finished()), this, SLOT(httpRequestFinished()));
}

void SeafileApiClient::onSslErrors(const QList<QSslError>& errors)
{
    const QUrl url = reply_->url();
    CertsManager *mgr = seafApplet->certsManager();
    Q_FOREACH(const QSslError &error, errors) {
        const QSslCertificate &cert = error.certificate();

        if (cert.isNull()) {
            // The server has no ssl certificate, we do nothing and let the
            // request fail
            // it is a fatal error, no way to recover
            qWarning("the certificate for %s is null", url.toString().toUtf8().data());
            break;
        }

        QSslCertificate saved_cert = mgr->getCertificate(url.toString());

        if (saved_cert.isNull()) {
            // dump certificate information
            qWarning() << "\n= SslError =\n" << error.errorString();
            qWarning() << dumpCipher(reply_->sslConfiguration().sessionCipher());
            qWarning() << dumpCertificate(cert);

            // This is the first time when the client connects to the server.
            if (seafApplet->detailedYesOrNoBox(
                tr("<b>Warning:</b> The ssl certificate of this server is not trusted, proceed anyway?"),
                error.errorString() + "\n" + dumpCertificate(cert), 0, false)) {
                mgr->saveCertificate(url, cert);
                // TODO handle ssl by verifying certificate chain instead
                reply_->ignoreSslErrors();
            }
            break;
        } else if (saved_cert == cert) {
            // The user has choosen to trust the certificate before
            // TODO handle ssl by verifying certificate chain instead
            reply_->ignoreSslErrors();
            break;
        } else {
            // dump certificate information
            qWarning() << "\n= SslError =\n" << error.errorString();
            qWarning() << dumpCipher(reply_->sslConfiguration().sessionCipher());
            qWarning() << dumpCertificate(cert);
            qWarning() << dumpCertificate(saved_cert);

            /**
             * The cert which the user had chosen to trust has been changed. It
             * may be either:
             *
             * 1. The server has changed its ssl certificate
             * 2. The user's connection is under security attack
             *
             * Anyway, we'll prompt the user
             */
            SslConfirmDialog dialog(url, cert, saved_cert, seafApplet->mainWindow());
            if (dialog.exec() == QDialog::Accepted) {
                // TODO handle ssl by verifying certificate chain instead
                reply_->ignoreSslErrors();
                if (dialog.rememberChoice()) {
                    mgr->saveCertificate(url, cert);
                }
            } else {
                reply_->abort();
                break;
            }
            break;
        }
    }
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

        NetworkStatusDetector::instance()->setNetworkFailure();

        if (!shouldIgnoreRequestError(reply_)) {
            qWarning("[api] network error for %s: %s\n", toCStr(reply_->url().toString()),
                   reply_->errorString().toUtf8().data());
        }
        emit networkError(reply_->error(), reply_->errorString());
        return;
    }
    NetworkStatusDetector::instance()->setNetworkSuccess();

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

void SeafileApiClient::setHeader(const QString& key, const QString& value)
{
    headers_[key] = value;
}
