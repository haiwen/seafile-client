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
    void switchMode();

private:
    Q_DISABLE_COPY(DownloadRepoDialog);

    bool validateInputsManualMergeMode();
    bool validateInputs();
    void setAllInputsEnabled(bool enabled);
    void setDirectoryText(const QString& path);
    void updateSyncMode();

    ServerRepo repo_;
    Account account_;

    // if we have enabled manual merge mode
    const bool has_manual_merge_mode_;

    // if we are in manual merge mode
    bool manual_merge_mode_;
    // used in auto mode, and always true in manual merge mode
    bool sync_with_existing_;
    // save merge path
    QString alternative_path_;
};
