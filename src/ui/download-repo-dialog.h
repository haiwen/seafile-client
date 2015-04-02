#include <QDialog>
#include <QUrl>
#include <QString>

#include "account.h"
#include "ui_download-repo-dialog.h"
#include "api/server-repo.h"

class RepoDownloadInfo;
class ApiError;

class DownloadRepoDialog : public QDialog,
                           public Ui::DownloadRepoDialog
{
    Q_OBJECT
public:
    DownloadRepoDialog(const Account& account,
                       const ServerRepo& repo,
                       QWidget *parent=0);

    void setMergeWithExisting(const QString& localPath);

private slots:
    void onOkBtnClicked();
    void chooseDirAction();
    void onDownloadRepoRequestSuccess(const RepoDownloadInfo& info);
    void onDownloadRepoRequestFailed(const ApiError& error);

private:
    Q_DISABLE_COPY(DownloadRepoDialog);

    bool validateInputs();
    void setAllInputsEnabled(bool enabled);
    void setDirectoryText(const QString& path);

    ServerRepo repo_;

    bool sync_with_existing_;

    Account account_;
};
