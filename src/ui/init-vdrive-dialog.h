#ifndef SEAFILE_CLIENT_INIT_VDRIVE_DIALOG_H
#define SEAFILE_CLIENT_INIT_VDRIVE_DIALOG_H

#include <QDialog>
#include "ui_init-vdrive-dialog.h"
#include "account.h"
#include "api/requests.h"

class LocalRepo;
class QTimer;

class InitVirtualDriveDialog : public QDialog,
                               public Ui::InitVirtualDriveDialog
{
    Q_OBJECT
public:
    InitVirtualDriveDialog(const Account& account, QWidget *parent=0);

private slots:
    void onGetDefaultRepoSuccess(bool exists, const QString& repo_id);
    void onGetDefaultRepoFailure(const ApiError& error);
    void onCreateDefaultRepoSuccess(const QString& repo_id);
    void onCreateDefaultRepoFailure(const ApiError& error);
    void onDownloadRepoSuccess(const RepoDownloadInfo& info);
    void onDownloadRepoFailure(const ApiError& error);
    void checkDownloadProgress();
    void start();
    void onCancel();
    void openVirtualDisk();

private:
    Q_DISABLE_COPY(InitVirtualDriveDialog)

    void getDefaultRepo();
    void startDownload(const QString& repo_id);
    void createLoadingView();
    void createDefaultRepo();
    void setStatusText(const QString& status);
    void setStatusIcon(const QString& path);
    void ensureVisible();
    void createVirtualDisk(const LocalRepo& repo);
    void finish();
    void fail(const QString& reason);

    GetDefaultRepoRequest *get_default_repo_req_;
    CreateDefaultRepoRequest *create_default_repo_req_;
    DownloadRepoRequest *download_default_repo_req_;

    QString default_repo_id_;
    QString default_repo_path_;

    Account account_;

    QTimer *check_download_timer_;
};

#endif // SEAFILE_CLIENT_INIT_VDRIVE_DIALOG_H
