#ifndef SEAFILE_CLIENT_SWITCH_ACCOUNT_DIALOG_H
#define SEAFILE_CLIENT_SWITCH_ACCOUNT_DIALOG_H

#include <QDialog>
#include "ui_switch-account-dialog.h"

#include "account.h"


class SwitchAccountDialog : public QDialog,
                            public Ui::SwitchAccountDialog
{
    Q_OBJECT

public:
    SwitchAccountDialog(QWidget *parent=0);

private slots:
    void submit();

signals:                                          
    void accountSelected(const Account& account);

private:
    void initAccountList();

    std::vector<Account> accounts_;
};

#endif // SEAFILE_CLIENT_SWITCH_ACCOUNT_DIALOG_H
