#include <QDialog>
#include <QUrl>
#include <QString>

#include "ui_create-repo-dialog.h"
#include "account.h"

class CreateRepoRequest;
class RepoDownloadInfo;
class ApiError;
class ReposTab;

class CreateRepoDialog : public QDialog,
                         public Ui::CreateRepoDialog
{
    Q_OBJECT
public:
    CreateRepoDialog(const Account& account,
                     const QString& worktree,
                     ReposTab *repos_tab,
                     QWidget *parent=0);

    ~CreateRepoDialog();

private slots:
    void createAction();
    void chooseDirAction();
    void createSuccess(const RepoDownloadInfo& info);
    void createFailed(const ApiError& error);

private:
    Q_DISABLE_COPY(CreateRepoDialog);
    bool validateInputs();
    void setAllInputsEnabled(bool enabled);
    QString translateErrorMsg(const QString& error);

    QString path_;
    QString name_;
    QString passwd_;
    CreateRepoRequest *request_;
    Account account_;
    ReposTab *repos_tab_;
};
