#ifndef SEAFILE_CLIENT_UI_ACTIVITIES_TAB_H
#define SEAFILE_CLIENT_UI_ACTIVITIES_TAB_H

#include <QList>

#include "tab-view.h"

class QSslError;
class QWebView;
class QTimer;
class QUrl;
class QNetworkRequest;
class QNetworkReply;

class Account;
class ApiError;

/**
 * The activities tab
 */
class ActivitiesTab : public TabView {
    Q_OBJECT
public:
    explicit ActivitiesTab(QWidget *parent=0);

public slots:
    void refresh();

private slots:
    void onLoadPageFinished(bool success);
    void onLinkClicked(const QUrl& url);
    void onDownloadRequested(const QNetworkRequest&);
    void handleSslErrors(QNetworkReply* reply, const QList<QSslError> &errors);

private:
    void createWebView();
    void createLoadingView();
    void createLoadingFailedView();
    void showLoadingView();
    void loadPage(const Account& account);

    QTimer *refresh_timer_;
    bool in_refresh_;

    QWebView *web_view_;
    QWidget *loading_view_;
    QWidget *loading_failed_view_;
};

#endif // SEAFILE_CLIENT_UI_ACTIVITIES_TAB_H
