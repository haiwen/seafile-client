#ifndef SEAFILE_CLIENT_ACCOUNT_SETTINGS_H
#define SEAFILE_CLIENT_ACCOUNT_SETTINGS_H

#include <QDialog>
#include "ui_account-settings-dialog.h"

#include <QUrl>
#include <QString>

#include "account.h"

class AccountSettingsDialog : public QDialog,
                              public Ui::AccountSettingsDialog
{
    Q_OBJECT
public:
    AccountSettingsDialog(const Account& account, QWidget *parent=0);

private slots:
    void onSubmitBtnClicked();

private:
    Q_DISABLE_COPY(AccountSettingsDialog)
    bool validateInputs();
    void showWarning(const QString& msg);

    Account account_;
};

#endif // SEAFILE_CLIENT_ACCOUNT_SETTINGS_H
