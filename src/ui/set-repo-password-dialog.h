#ifndef SEAFILE_CLIENT_SET_REPO_PASSWORD_DIALOG_H
#define SEAFILE_CLIENT_SET_REPO_PASSWORD_DIALOG_H

#include <QDialog>
#include <QString>
#include "ui_set-repo-password-dialog.h"

#include "api/server-repo.h"

class Account;
class ApiError;
class SetRepoPasswordRequest;

class SetRepoPasswordDialog : public QDialog,
                              public Ui::SetRepoPasswordDialog
{
    Q_OBJECT
public:
    SetRepoPasswordDialog(const ServerRepo& repo, QWidget *parent=0);
    const QString& password() { return password_; }

private slots:
    void onOkBtnClicked();
    void requestFailed(const ApiError& error);

private:
    Q_DISABLE_COPY(SetRepoPasswordDialog);

    void enableInputs();
    void disableInputs();

    QString password_;
    SetRepoPasswordRequest *request_;
    ServerRepo repo_;
};

#endif // SEAFILE_CLIENT_SET_REPO_PASSWORD_DIALOG_H
