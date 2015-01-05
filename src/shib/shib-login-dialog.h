#ifndef SEAFILE_CLIENT_SHIB_LOGIN_DIALOG_H
#define SEAFILE_CLIENT_SHIB_LOGIN_DIALOG_H

#include <QDialog>
#include <QUrl>
#include <QNetworkCookieJar>

#include "account.h"

template<typename T> class QList;

class QWebView;
class QSslError;
class QNetworkReply;

/**
 * Login with Shibboleth SSO.
 *
 * This dialog use a webview to let the user login seahub configured with
 * Shibboleth SSO auth. When the login succeeded, seahub would set the
 * username and api token in the cookie.
 */
class ShibLoginDialog : public QDialog {
    Q_OBJECT
public:
    ShibLoginDialog(const QUrl& url, QWidget *parent=0);

private slots:
    void sslErrorHandler(QNetworkReply* reply, const QList<QSslError> & ssl_errors);
    void onNewCookieCreated(const QUrl& url, const QNetworkCookie& cookie);

private:
    Account parseAccount(const QString& txt);
    
    QWebView *webview_;
    QUrl url_;
};


/**
 * Wraps the standard Qt cookie jar to emit a signal when new cookies created.
 */
class CustomCookieJar : public QNetworkCookieJar
{
    Q_OBJECT
public:
    explicit CustomCookieJar(QObject *parent = 0);
    bool setCookiesFromUrl(const QList<QNetworkCookie>& cookies, const QUrl& url);

signals:
    void newCookieCreated(const QUrl& url, const QNetworkCookie& cookie);
};

#endif /* SEAFILE_CLIENT_SHIB_LOGIN_DIALOG_H */
