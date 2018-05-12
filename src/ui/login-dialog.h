#ifndef SEAFILE_CLIENT_LOGIN_DIALOG_H
#define SEAFILE_CLIENT_LOGIN_DIALOG_H

#include <QDialog>
#include "ui_login-dialog.h"

#include <QUrl>
#include <QString>
#include <QNetworkReply>

#include "two-factor-dialog.h"

class Account;
class LoginRequest;
class QNetworkReply;
class QSslError;
class ApiError;
class FetchAccountInfoRequest;
class AccountInfo;

class LoginDialog : public QDialog,
                    public Ui::LoginDialog
{
    Q_OBJECT
public:
    LoginDialog(QWidget *parent=0);
    void initFromAccount(const Account& account);

private slots:
    void doLogin();
    void loginSuccess(const QString& token, const QString& s2fa_token);
    void loginFailed(const ApiError& error);
#ifdef HAVE_SHIBBOLETH_SUPPORT
    void loginWithShib();
#endif // HAVE_SHIBBOLETH_SUPPORT
    void onFetchAccountInfoSuccess(const AccountInfo& info);
    void onFetchAccountInfoFailed(const ApiError&);
    void startNewSSO();

private:
    Q_DISABLE_COPY(LoginDialog);

    enum LoginMode {
        LOGIN_NORMAL = 0,
        LOGIN_SHIB
    };

    void setupShibLoginLink();
    bool validateInputs();
    void disableInputs();
    void enableInputs();
    void showWarning(const QString& msg);

    void onNetworkError(const QNetworkReply::NetworkError& error, const QString& error_string);
    void onSslErrors(QNetworkReply *reply, const QList<QSslError>& errors);
    void onHttpError(int code);
    bool getSSOLoginUrl(const QString& last_shib_url, QUrl *url_out);

    QUrl url_;
    QString username_;
    QString password_;
    QString computer_name_;
    bool is_remember_device_;
    LoginRequest *request_;
    FetchAccountInfoRequest *account_info_req_;

    QString two_factor_auth_token_;

#ifdef HAVE_SHIBBOLETH_SUPPORT
    LoginMode mode_;
#endif // HAVE_SHIBBOLETH_SUPPORT
};

#endif // SEAFILE_CLIENT_LOGIN_DIALOG_H
