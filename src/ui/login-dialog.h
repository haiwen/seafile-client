#ifndef SEAFILE_CLIENT_LOGIN_DIALOG_H
#define SEAFILE_CLIENT_LOGIN_DIALOG_H

#include <QDialog>
#include "ui_login-dialog.h"

#include <QUrl>
#include <QString>
#include <QNetworkReply>

class Account;
class LoginRequest;
class QNetworkReply;
class QSslError;
class ApiError;

class LoginDialog : public QDialog,
                    public Ui::LoginDialog
{
    Q_OBJECT
public:
    LoginDialog(QWidget *parent=0);

private slots:
    void doLogin();
    void loginSuccess(const QString& token);
    void loginFailed(const ApiError& error);
    void loginWithShib();

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

    QUrl url_;
    QString username_;
    QString password_;
    QString computer_name_;
    LoginRequest *request_;

    LoginMode mode_;
};

#endif // SEAFILE_CLIENT_LOGIN_DIALOG_H
