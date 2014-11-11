#include <QDialog>
#include <QUrl>
#include <QString>

#include "ui_create-repo-dialog.h"
#include "account.h"

class CreateRepoRequest;
class RepoDownloadInfo;
class ApiError;

class CreateRepoDialog : public QDialog,
                         public Ui::CreateRepoDialog
{
    Q_OBJECT
public:
    CreateRepoDialog(const Account& account,
                     const QString& worktree,
                     QWidget *parent=0);

    ~CreateRepoDialog();

private slots:
    void createAction();
    void chooseDirAction();
    void createSuccess(const RepoDownloadInfo& info);
    void createFailed(const ApiError& error);

private:
    Q_DISABLE_COPY(CreateRepoDialog)
    bool validateInputs();
    void setAllInputsEnabled(bool enabled);

    QString path_;
    QString name_;
    QString desc_;
    QString passwd_;
    CreateRepoRequest *request_;
    Account account_;
};
