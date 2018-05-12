#ifndef SEAFILE_CLIENT_SSO_DIALOG_H
#define SEAFILE_CLIENT_SSO_DIALOG_H

#include <QDialog>
#include <QScopedPointer>
#include "ui_sso-dialog.h"

#include <QNetworkReply>
#include <QString>
#include <QUrl>

#include "api/requests.h"

class Account;
class SSORequest;
class QNetworkReply;
class QSslError;
class ApiError;
class FetchAccountInfoRequest;
class AccountInfo;
class closeEvent;

/**
 * Login with new seahub SSO.
 *
 * This dialog is used to display the login progress.
 */
class SSODialog : public QDialog, public Ui::SSODialog
{
    Q_OBJECT
public:
    SSODialog(QWidget *parent = 0);

private slots:
    void start();
    void getSSOLink();
    void onGetSSOLinkSuccess(const QString &link);
    void onGetSSOLinkFailed(const ApiError &error);
    void onSSOSuccess(const QString &status,
                      const QString &email,
                      const QString &apikey);
    void onSSOFailed(const ApiError &error);
    void ssoCheckTimerCB();
    void onFetchAccountInfoSuccess(const AccountInfo &info);
    void onFetchAccountInfoFailed(const ApiError &);
    void openSSOLoginLinkInBrowser();
    void closeEvent(QCloseEvent *event);

private:
    Q_DISABLE_COPY(SSODialog);
    void fail(const QString &msg);
    void showWarning(const QString& msg);
    void setStatusText(const QString &status);
    void setStatusIcon(const QString &path);
    void ensureVisible();
    void startSSOStatusCheck();
    void sendSSOStatusRequest();
    void getAccountInfo(const Account &account);
    bool getSSOLoginUrl(const QString& last_url, QUrl *url_out);

    QUrl server_addr_;
    QString computer_name_;
    QScopedPointer<GetSSOLinkRequest, QScopedPointerDeleteLater>
        get_link_request_;
    QUrl sso_login_link_;
    QUrl sso_status_link_;

    QTimer *sso_check_timer_;
    QScopedPointer<GetSSOStatusRequest, QScopedPointerDeleteLater>
        status_check_request_;

    QScopedPointer<FetchAccountInfoRequest, QScopedPointerDeleteLater>
        account_info_req_;
};

#endif // SEAFILE_CLIENT_SSO_DIALOG_H
