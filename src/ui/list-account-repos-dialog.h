#ifndef LIST_ACCOUNT_REPOS_DIALOG_H
#define LIST_ACCOUNT_REPOS_DIALOG_H

#include <vector>
#include <QDialog>
#include "ui_list-account-repos-dialog.h"
#include "account.h"
#include "api/server-repo.h"
#include "rpc/rpc-client.h"

class ServerRepo;
class ListReposRequest;
class QListWidget;

class ListAccountReposDialog : public QDialog,
                              public Ui::ListAccountReposDialog
{
    Q_OBJECT
    Q_DISABLE_COPY(ListAccountReposDialog)

public:
    ListAccountReposDialog(const Account& account, QWidget *parent=0);
    ~ListAccountReposDialog();
    void setAccount(const Account& account) { account_ = account_; }

private slots:
    void onListApiSuccess(const std::vector<ServerRepo>& repos);
    void onListApiFailed(int);
    void onDownloadApiSuccess(const QMap<QString, QString> &dict, ServerRepo *repo);
    void onDownloadApiFailed(int code, ServerRepo *repo);

    void downloadRepoRequestFinished(QString &repoId, bool result);
    void cloneRepoRequestFinished(QString &repoId, bool result);

    void syncRepoAction();
    void createRepoAction();

private:
    void sendListReposRequest();

    Account account_;
    ListReposRequest *request_;
    std::vector<ServerRepo> repos_;

    QListWidget *list_widget_;
};


#endif // LIST_ACCOUNT_REPOS_DIALOG_H
