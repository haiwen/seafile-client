#ifndef SEAFILE_CLIENT_CHECK_REPO_ROOT_PERM_DIALOG_H
#define SEAFILE_CLIENT_CHECK_REPO_ROOT_PERM_DIALOG_H

#include <QUrl>
#include <QString>
#include <QProgressDialog>

#include "api/server-repo.h"
#include "account.h"
#include "filebrowser/file-browser-dialog.h"

class ApiError;
class DataManager;
class SeafDirent;

// When the user drag'n'drop a file into to a repo and the repo is readonly, we
// need to check the user's directory-level permission for the repo's root
// directory.
//
// This dialog would be showed to the user when we send the request to the server.
//
class CheckRepoRootDirPermDialog : public QProgressDialog
{
    Q_OBJECT
public:
    CheckRepoRootDirPermDialog(const Account &account,
                               const ServerRepo& repo,
                               const QString& local_path,
                               QWidget *parent = 0);
    ~CheckRepoRootDirPermDialog();

    QString localPath() const { return local_path_; }
    ServerRepo repo() const { return repo_; }

    bool hasWritePerm() const { return has_write_perm_; }

private slots:
    void checkPerm();
    void onGetDirentsSuccess(bool readonly);
    void onGetDirentsFailed();

private:
    Q_DISABLE_COPY(CheckRepoRootDirPermDialog);

    Account account_;
    ServerRepo repo_;
    QString local_path_;

    bool has_write_perm_;

    QLabel *description_label_;
    QLabel *more_details_label_;
    QProgressBar *progress_bar_;
    DataManager *data_mgr_;
    DataManagerNotify *data_mgr_notify_;
};

#endif // SEAFILE_CLIENT_CHECK_REPO_ROOT_PERM_DIALOG_H
