#include <cstdio>
#include <QtGui>
#include <QTimer>
#include <QIcon>
#include <QNetworkRequest>
#include <QStackedWidget>
#include <QWebFrame>
#include <QSslError>

#include "seafile-applet.h"
#include "account-mgr.h"
#include "webview.h"

#include "activities-tab.h"

namespace {

const int kRefreshInterval = 1000 * 60 * 5; // 5 min
const char *kLoadingFaieldLabelName = "loadingFailedText";
const char *kEmptyViewLabelName = "emptyText";
const char *kAuthHeader = "Authorization";
const char *kActivitiesUrl = "/api2/html/events/";

enum {
    INDEX_LOADING_VIEW = 0,
    INDEX_LOADING_FAILED_VIEW,
    INDEX_WEB_VIEW,
};

}


ActivitiesTab::ActivitiesTab(QWidget *parent)
    : TabView(parent),
      in_refresh_(false)
{
    createWebView();
    createLoadingView();
    createLoadingFailedView();

    mStack->insertWidget(INDEX_LOADING_VIEW, loading_view_);
    mStack->insertWidget(INDEX_LOADING_FAILED_VIEW, loading_failed_view_);
    mStack->insertWidget(INDEX_WEB_VIEW, web_view_);

    refresh_timer_ = new QTimer(this);
    connect(refresh_timer_, SIGNAL(timeout()), this, SLOT(refresh()));
    refresh_timer_->start(kRefreshInterval);

    refresh();
}

void ActivitiesTab::refresh()
{
    if (in_refresh_) {
        return;
    }

    in_refresh_ = true;

    showLoadingView();
    AccountManager *account_mgr = seafApplet->accountManager();

    const std::vector<Account>& accounts = seafApplet->accountManager()->accounts();
    if (accounts.empty()) {
        in_refresh_ = false;
        return;
    }

    loadPage(accounts[0]);
}

void ActivitiesTab::loadPage(const Account& account)
{
    QNetworkRequest request;
    QUrl url(account.serverUrl);
    url.setPath(url.path() + kActivitiesUrl);

    printf("url is %s\n", url.toString().toUtf8().data());
    request.setUrl(url);
    QString token_header = QString("Token %1").arg(account.token);
    request.setRawHeader(kAuthHeader, token_header.toUtf8().data());

    web_view_->load(request);
}

void ActivitiesTab::createWebView()
{
    web_view_ = new WebView;
    QWebPage *page = web_view_->page();
    page->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);

    QNetworkAccessManager *oldManager = page->networkAccessManager();
    NetworkAccessManager *newManager = new NetworkAccessManager(oldManager, this);
    page->setNetworkAccessManager(newManager);
    page->setForwardUnsupportedContent(true);

    
    connect(newManager,
            SIGNAL(sslErrors(QNetworkReply*, const QList<QSslError>&)),
            this, SLOT(handleSslErrors(QNetworkReply*, const QList<QSslError>&)));
    
    // QScrollArea *scroll = new QScrollArea;
    // scroll->setWidget(web_view_);
    // mStack->insertWidget(INDEX_WEB_VIEW, scroll);
    mStack->insertWidget(INDEX_WEB_VIEW, web_view_);

    connect(web_view_, SIGNAL(loadFinished(bool)),
            this, SLOT(onLoadPageFinished(bool)));
    connect(page, SIGNAL(linkClicked(const QUrl&)),
            this, SLOT(onLinkClicked(const QUrl&)));
    connect(page, SIGNAL(downloadRequested(const QNetworkRequest&)),
            this, SLOT(onDownloadRequested(const QNetworkRequest&)));
}

void ActivitiesTab::createLoadingView()
{
    loading_view_ = new QWidget(this);

    QVBoxLayout *layout = new QVBoxLayout;
    loading_view_->setLayout(layout);

    QMovie *gif = new QMovie(":/images/loading.gif");
    QLabel *label = new QLabel;
    label->setMovie(gif);
    label->setAlignment(Qt::AlignCenter);
    gif->start();

    layout->addWidget(label);
}

void ActivitiesTab::createLoadingFailedView()
{
    loading_failed_view_ = new QWidget(this);

    QVBoxLayout *layout = new QVBoxLayout;
    loading_failed_view_->setLayout(layout);

    QLabel *label = new QLabel;
    label->setObjectName(kLoadingFaieldLabelName);
    QString link = QString("<a style=\"color:#777\" href=\"#\">%1</a>").arg(tr("retry"));
    QString label_text = tr("Failed to get actvities information<br/>"
                            "Please %1").arg(link);
    label->setText(label_text);
    label->setAlignment(Qt::AlignCenter);

    connect(label, SIGNAL(linkActivated(const QString&)),
            this, SLOT(refresh()));

    layout->addWidget(label);
}

void ActivitiesTab::showLoadingView()
{
    mStack->setCurrentIndex(INDEX_LOADING_VIEW);
}

void ActivitiesTab::onLoadPageFinished(bool success)
{
    in_refresh_ = false;
    if (success) {
        printf("webview page load success\n");
        mStack->setCurrentIndex(INDEX_WEB_VIEW);
        Account account = seafApplet->accountManager()->currentAccount();
        QString js = QString("setToken('%1')").arg(account.token);
        web_view_->page()->mainFrame()->evaluateJavaScript(js);
    } else {
        printf("webview page load failed\n");
        mStack->setCurrentIndex(INDEX_LOADING_FAILED_VIEW);
    }
}

void ActivitiesTab::onLinkClicked(const QUrl& url)
{
    printf("link is clicked: %s\n", url.toString().toUtf8().data());
}

void ActivitiesTab::onDownloadRequested(const QNetworkRequest& request)
{
    printf("download url is %s\n", request.url().toString().toUtf8().data());
}

void ActivitiesTab::handleSslErrors(QNetworkReply* reply, const QList<QSslError> &errors)
{
    // TODO: handle ssl error
    qDebug() << "handleSslErrors: ";
    foreach (QSslError e, errors)
    {
        qDebug() << "ssl error: " << e;
    }

    reply->ignoreSslErrors();
}
