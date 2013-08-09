#ifndef GET_ACCOUNT_REPOS_DIALOG_H
#define GET_ACCOUNT_REPOS_DIALOG_H

#include <vector>
#include <QDialog>
#include "ui_get-account-repos-dialog.h"
#include "account.h"

class SeafRepo;

class GetAccountReposDialog : public QDialog,
                              public Ui::GetAccountReposDialog
{
    Q_OBJECT
    Q_DISABLE_COPY(GetAccountReposDialog)

public:
    GetAccountReposDialog(const Account& account, QWidget *parent=0);
    void setAccount(const Account& account) { account_ = account_; }

private slots:
    void onRequestSuccess(const std::vector<SeafRepo>& repos);
    void onRequestFailed();

private:
    Account account_;
};


#endif // GET_ACCOUNT_REPOS_DIALOG_H
