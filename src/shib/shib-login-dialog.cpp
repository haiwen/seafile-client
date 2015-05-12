#include <QtGui>
#include <QWebView>
#include <QVBoxLayout>
#include <QList>
#include <QSslError>
#include <QNetworkReply>
#include <QNetworkCookie>

#include "seafile-applet.h"
#include "utils/utils.h"
#include "utils/api-utils.h"
#include "account-mgr.h"
#include "network-mgr.h"

#include "shib-login-dialog.h"

namespace {

const char *kSeahubShibCookieName = "seahub_auth";

} // namespace

ShibLoginDialog::ShibLoginDialog(const QUrl& url,
                                 const QString& computer_name,
                                 QWidget *parent)
    : QDialog(parent),
      url_(url),
      cookie_seen_(false)
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
    NetworkManager::instance()->addWatch(mgr);
    mgr->setCookieJar(jar);

    connect(mgr, SIGNAL(sslErrors(QNetworkReply*, const QList<QSslError>&)),
            this, SLOT(sslErrorHandler(QNetworkReply*, const QList<QSslError>&)));

    connect(jar, SIGNAL(newCookieCreated(const QUrl&, const QNetworkCookie&)),
            this, SLOT(onNewCookieCreated(const QUrl&, const QNetworkCookie&)));

    QUrl shib_login_url(url_);
    QString path = shib_login_url.path();
    if (!path.endsWith("/")) {
        path += "/";
    }
    path += "shib-login";
    shib_login_url.setPath(path);

    webview_->load(::includeQueryParams(
                       shib_login_url, ::getSeafileLoginParams(computer_name, "shib_")));
}


void ShibLoginDialog::sslErrorHandler(QNetworkReply* reply,
                                      const QList<QSslError> & ssl_errors)
{
    reply->ignoreSslErrors();
}

void ShibLoginDialog::onNewCookieCreated(const QUrl& url, const QNetworkCookie& cookie)
{
    if (cookie_seen_) {
        return;
    }
    QString name = cookie.name();
    QString value = cookie.value();
    if (url.host() == url_.host() && name == kSeahubShibCookieName) {
        Account account = parseAccount(value);
        if (!account.isValid()) {
            qWarning("wrong account information from server");
            return;
        }
        cookie_seen_ = true;
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
    if (email.isEmpty() or token.isEmpty()) {
        return Account();
    }
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
