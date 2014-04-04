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

private slots:
    void onOkBtnClicked();
    void chooseDirAction();
    void onDownloadRepoRequestSuccess(const RepoDownloadInfo& info);
    void onDownloadRepoRequestFailed(const ApiError& error);
    void switchMode();

private:
    Q_DISABLE_COPY(DownloadRepoDialog);

    enum SyncMode {
        CREATE_NEW_FOLDER,
        MERGE_WITH_EXISTING_FOLDER
    };
    
    
    bool validateInputs();
    void setAllInputsEnabled(bool enabled);
    void setDirectoryText(const QString& path);
    void updateSyncMode();

    ServerRepo repo_;

    bool sync_with_existing_;

    Account account_;

    SyncMode mode_;

    QString saved_create_new_path_, saved_merge_existing_path_;
};
