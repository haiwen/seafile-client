#include <QtGui>
#include <QWebView>
#include <QVBoxLayout>
#include <QList>
#include <QSslError>
#include <QNetworkReply>
#include <QNetworkCookie>

#include "shib-login-dialog.h"
#include "seafile-applet.h"
#include "account-mgr.h"

namespace {

const char *kSeahubShibCookieName = "seahub_auth";

} // namespace

ShibLoginDialog::ShibLoginDialog(const QUrl& url, QWidget *parent)
    : QDialog(parent),
      url_(url)
{
    setWindowTitle(tr("Login with Shibboleth"));
    setWindowIcon(QIcon(":/images/seafile.png"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    QVBoxLayout *vlayout = new QVBoxLayout();
    setLayout(vlayout);

    webview_ = new QWebView;
    vlayout->addWidget(webview_);

    CustomCookieJar *jar = new CustomCookieJar(this);
    QNetworkAccessManager *mgr = webview_->page()->networkAccessManager();
    mgr->setCookieJar(jar);

    connect(mgr, SIGNAL(sslErrors(QNetworkReply*, const QList<QSslError>&)),
            this, SLOT(sslErrorHandler(QNetworkReply*, const QList<QSslError>&)));

    connect(jar, SIGNAL(newCookieCreated(const QUrl&, const QNetworkCookie&)),
            this, SLOT(onNewCookieCreated(const QUrl&, const QNetworkCookie&)));

    webview_->load(url_);
}


void ShibLoginDialog::sslErrorHandler(QNetworkReply* reply,
                                      const QList<QSslError> & ssl_errors)
{
    reply->ignoreSslErrors();
}

void ShibLoginDialog::onNewCookieCreated(const QUrl& url, const QNetworkCookie& cookie)
{
    QString name = cookie.name();
    if (url.host() == url_.host() && name == kSeahubShibCookieName) {
        QString value = cookie.value();

        Account account = parseAccount(value);
        if (seafApplet->accountManager()->saveAccount(account) < 0) {
            seafApplet->warningBox(tr("Failed to save current account"), this);
            reject();
        } else {
            accept();
        }
    }
}


/**
 * The cookie value is like seahub_shib="foo@test.com@bd8cc1138", where
 * foo@test.com is username and bd8cc1138 is api token"
 */
Account ShibLoginDialog::parseAccount(const QString& cookie_value)
{
    QString txt = cookie_value;
    if (txt.startsWith("\"")) {
        txt = txt.mid(1, txt.length() - 2);
    }
    int pos = txt.lastIndexOf("@");
    QString email = txt.left(pos);
    QString token = txt.right(txt.length() - pos - 1);
    return Account(url_, email, token);
}


CustomCookieJar::CustomCookieJar(QObject *parent)
    : QNetworkCookieJar(parent)
{
}

bool CustomCookieJar::setCookiesFromUrl(const QList<QNetworkCookie>& cookies, const QUrl& url)
{
  if (QNetworkCookieJar::setCookiesFromUrl(cookies, url)) {
      foreach (const QNetworkCookie& cookie, cookies) {
          emit newCookieCreated(url, cookie);
      }
    return true;
  }

  return false;
}
