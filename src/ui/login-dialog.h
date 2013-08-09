#include <QDialog>
#include "ui_login-dialog.h"

#include <QUrl>
#include <QString>

class Account;

class LoginDialog : public QDialog,
                    public Ui::LoginDialog
{
    Q_OBJECT
public:
    LoginDialog(QWidget *parent=0);

private slots:
    void doLogin();
    void loginSuccess(const Account& account);
    void loginFailed();

private:
    Q_DISABLE_COPY(LoginDialog);
    bool validateInputs();

    QUrl url_;
    QString username_;
    QString password_;
};
