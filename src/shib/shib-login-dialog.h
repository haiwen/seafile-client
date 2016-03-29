#ifndef SEAFILE_CLIENT_SHIB_LOGIN_DIALOG_H
#define SEAFILE_CLIENT_SHIB_LOGIN_DIALOG_H

#include <QDialog>
#include <QUrl>
#include <QNetworkCookieJar>

#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
#include <QWebEnginePage>
#endif

#include "account.h"

template<typename T> class QList;

#if (QT_VERSION < QT_VERSION_CHECK(5, 6, 0))
class QWebView;
#else
class QWebEngineView;
#endif

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
    ShibLoginDialog(const QUrl& url,
                    const QString& computer_name,
                    QWidget *parent=0);

private slots:
    void sslErrorHandler(QNetworkReply* reply, const QList<QSslError> & ssl_errors);
    void onNewCookieCreated(const QUrl& url, const QNetworkCookie& cookie);
    void onWebEngineCookieAdded(const QNetworkCookie& cookie);

private:
    Account parseAccount(const QString& txt);

#if (QT_VERSION < QT_VERSION_CHECK(5, 6, 0))
    QWebView *webview_;
#else
    QWebEngineView *webview_;
#endif
    QUrl url_;
    bool cookie_seen_;
};


#if (QT_VERSION < QT_VERSION_CHECK(5, 6, 0))

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

#else

class QWebEngineCertificateError;
class SeafileQWebEnginePage : public QWebEnginePage
{
    Q_OBJECT
public:
    SeafileQWebEnginePage(QObject *parent = 0);

protected:
    bool certificateError(
        const QWebEngineCertificateError &certificateError);
};


#endif

#endif /* SEAFILE_CLIENT_SHIB_LOGIN_DIALOG_H */
