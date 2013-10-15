#ifndef SEAFILE_CLIENT_LOGIN_DIALOG_H
#define SEAFILE_CLIENT_LOGIN_DIALOG_H

#include <QDialog>
#include "ui_login-dialog.h"

#include <QUrl>
#include <QString>

struct Account;
class LoginRequest;

class LoginDialog : public QDialog,
                    public Ui::LoginDialog
{
    Q_OBJECT
public:
    LoginDialog(QWidget *parent=0);

private slots:
    void doLogin();
    void loginSuccess(const QString& token);
    void loginFailed(int code);

private:
    Q_DISABLE_COPY(LoginDialog);
    bool validateInputs();

    QUrl url_;
    QString username_;
    QString password_;
    LoginRequest *request_;
};

#endif // SEAFILE_CLIENT_LOGIN_DIALOG_H
