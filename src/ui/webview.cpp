#include <QNetworkReply>
#include <QtDebug>
#include <QSslError>
#include <QNetworkProxy>

#include "webview.h"

WebView::WebView(QWidget *parent) :
    QWebView(parent)
{
    connect(page()->networkAccessManager(),
            SIGNAL(sslErrors(QNetworkReply*, const QList<QSslError>&)),
            this, SLOT(handleSslErrors(QNetworkReply*, const QList<QSslError>&)));
}

void WebView::handleSslErrors(QNetworkReply* reply, const QList<QSslError> &errors)
{
    // TODO: handle ssl error
    qDebug() << "handleSslErrors: ";
    foreach (QSslError e, errors)
    {
        qDebug() << "ssl error: " << e;
    }

    reply->ignoreSslErrors();
}

NetworkAccessManager::NetworkAccessManager(QNetworkAccessManager *manager, QObject *parent)
    : QNetworkAccessManager(parent)
{
    setCache(manager->cache());
    setCookieJar(manager->cookieJar());
    setProxy(manager->proxy());
    setProxyFactory(manager->proxyFactory());
}

QNetworkReply*
NetworkAccessManager::createRequest(QNetworkAccessManager::Operation operation,
                                    const QNetworkRequest &request,
                                    QIODevice *device)
{
    printf("createRequest: url is %s\n", request.url().toString().toUtf8().data());
    QString scheme = request.url().scheme();
    if (scheme == "http" || scheme == "https") {
        return QNetworkAccessManager::createRequest(operation, request, device);
    }

    return new BlobReply(request.url());
}

BlobReply::BlobReply(const QUrl &url)
    : QNetworkReply()
{
}

void BlobReply::abort()
{
}

qint64 BlobReply::readData(char *data, qint64 maxSize)
{
    return -1;
}
