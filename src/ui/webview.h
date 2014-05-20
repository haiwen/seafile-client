#ifndef SEAFILE_UI_WEBVIEW_H
#define SEAFILE_UI_WEBVIEW_H

#include <QWebView>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class QNetworkReply;

class WebView : public QWebView
{
    Q_OBJECT
public:
    explicit WebView(QWidget *parent = 0);
private slots:
    void handleSslErrors(QNetworkReply* reply, const QList<QSslError> &errors);
};

class NetworkAccessManager : public QNetworkAccessManager
{

    Q_OBJECT
public:
    explicit NetworkAccessManager(QNetworkAccessManager *manager, QObject *parent);

protected:
    QNetworkReply* createRequest(QNetworkAccessManager::Operation operation,
                                 const QNetworkRequest &request,
                                 QIODevice *device);
};

class BlobReply : public QNetworkReply
{
    Q_OBJECT
public:
    explicit BlobReply(const QUrl &url);
    void abort();
protected:
    qint64 readData(char *data, qint64 maxSize);
};


#endif // SEAFILE_UI_WEBVIEW_H
