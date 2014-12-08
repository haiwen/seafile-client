#ifndef SEAFILE_CLIENT_FILE_BROWSER_DIALOG_H
#define SEAFILE_CLIENT_FILE_BROWSER_DIALOG_H

#include <QStack>
#include <QDialog>
// #include "ui_file-browser-dialog.h"

#include "api/server-repo.h"

class QToolBar;
class QAction;
class QStackedWidget;
class QLineEdit;
class QLabel;
class QButtonGroup;

class ApiError;
class FileTableView;
class FileTableModel;
class SeafDirent;
class GetDirentsRequest;
class FileBrowserCache;
class DataManager;
class FileNetworkTask;

/**
 * This dialog is used when the user clicks on a repo not synced yet.
 *
 * The user can browse the files of this repo. When he clicks on a file, the
 * file would be downloaded to a temporary location. If the user modifies the
 * downloaded file, the new version would be automatically uploaded to the
 * server.
 *
 */
class FileBrowserDialog : public QDialog
                          // public Ui::FileBrowserDialog
{
    Q_OBJECT
public:
    FileBrowserDialog(const ServerRepo& repo,
                      QWidget *parent=0);
    ~FileBrowserDialog();

    friend class FileTableView;
    friend class FileTableModel;

private slots:
    void onGetDirentsSuccess(const QList<SeafDirent>& dirents);
    void onGetDirentsFailed(const ApiError& error);
    void onMkdirButtonClicked();
    void fetchDirents();
    void onDirentClicked(const SeafDirent& dirent);
    void forceRefresh();
    void goForward();
    void goBackward();
    void goHome();
    void chooseFileToUpload();
    void onDownloadFinished(bool success);
    void onUploadFinished(bool success);
    void openCacheFolder();

    // prompt a dialog for user to choose whether upload or update
    void uploadOrUpdateFile(const QString& path);

    void onNavigatorClick(int id);

    void onGetDirentRename(const SeafDirent& dirent, QString new_name = QString());
    void onGetDirentRemove(const SeafDirent& dirent);
    void onGetDirentShare(const SeafDirent& dirent);
    void onGetDirentUpdate(const SeafDirent& dirent);
    void onCancelDownload(const SeafDirent& dirent);

    void onDirectoryCreateSuccess(const QString& path);
    void onDirectoryCreateFailed(const ApiError& error);
    void onDirentRenameSuccess(const QString& path, const QString& new_name);
    void onDirentRenameFailed(const ApiError& error);
    void onDirentRemoveSuccess(const QString& path);
    void onDirentRemoveFailed(const ApiError& error);
    void onDirentShareSuccess(const QString& link);
    void onDirentShareFailed(const ApiError& error);
    void onFileAutoUpdated(const QString& repo_id, const QString& path);

private:
    Q_DISABLE_COPY(FileBrowserDialog)

    void createToolBar();
    void createStatusBar();
    void createFileTable();
    void createLoadingFailedView();
    void showLoading();
    void updateTable(const QList<SeafDirent>& dirents);
    void enterPath(const QString& path);
    void createDirectory(const QString &name);
    void downloadFile(const QString& path);
    void uploadFile(const QString& path, const QString& name,
                    const bool overwrite = false);

    void onDirClicked(const SeafDirent& dirent);
    void onFileClicked(const SeafDirent& dirent);

    void fetchDirents(bool force_refresh);

    bool setPasswordAndRetry(FileNetworkTask *task);

    ServerRepo repo_;
    // current path
    QString current_path_;
    QStringList current_lpath_;
    QStack<QString> forward_history_;
    QStack<QString> backward_history_;

    QToolBar *toolbar_;
    QAction *backward_action_;
    QAction *forward_action_;
    QButtonGroup *path_navigator_;
    QList<QLabel*> path_navigator_separators_;
    QAction *gohome_action_;
    QAction *refresh_action_;

    QToolBar *status_bar_;
    QAction *upload_action_;
    QAction *mkdir_action_;
    QLabel *details_label_;
    QAction *open_cache_dir_action_;

    QStackedWidget *stack_;
    QWidget *loading_view_;
    QWidget *loading_failed_view_;
    FileTableView *table_view_;
    FileTableModel *table_model_;

    DataManager *data_mgr_;
};


#endif  // SEAFILE_CLIENT_FILE_BROWSER_DIALOG_H
