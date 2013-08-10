#ifndef LIST_ACCOUNT_REPOS_DIALOG_H
#define LIST_ACCOUNT_REPOS_DIALOG_H

#include <vector>
#include <QDialog>
#include "ui_list-account-repos-dialog.h"
#include "account.h"

class SeafRepo;
class ListReposRequest;
class QListWidget;

class ListAccountReposDialog : public QDialog,
                              public Ui::ListAccountReposDialog
{
    Q_OBJECT
    Q_DISABLE_COPY(ListAccountReposDialog)

public:
    ListAccountReposDialog(const Account& account, QWidget *parent=0);
    void setAccount(const Account& account) { account_ = account_; }

private slots:
    void onRequestSuccess(const std::vector<SeafRepo>& repos);
    void onRequestFailed(int);

private:
    void sendRequest();

    Account account_;
    ListReposRequest *request_;

    QListWidget *list_widget_;
};


#endif // LIST_ACCOUNT_REPOS_DIALOG_H
