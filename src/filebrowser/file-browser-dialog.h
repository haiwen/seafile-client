#ifndef SEAFILE_CLIENT_FILE_BROWSER_DIALOG_H
#define SEAFILE_CLIENT_FILE_BROWSER_DIALOG_H

#include <QStack>
#include <QDialog>

#include "account.h"
#include "api/server-repo.h"

class QToolBar;
class QToolButton;
class QAction;
class QStackedWidget;
class QLineEdit;
class QLabel;
class QButtonGroup;
class QMenu;
class QAction;

class ApiError;
class FileTableView;
class FileTableModel;
class SeafDirent;
class GetDirentsRequest;
class FileBrowserCache;
class DataManager;
class FileNetworkTask;
class FileBrowserManager;

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
    friend class FileBrowserManager;
public:
    FileBrowserDialog(const Account &account, const ServerRepo& repo,
                      QWidget *parent=0);
    ~FileBrowserDialog();

    friend class FileTableView;
    friend class FileTableModel;
signals:
    void aboutToClose();

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
    void chooseDirectoryToUpload();
    void onDownloadFinished(bool success);
    void onUploadFinished(bool success);
    void openCacheFolder();

    // prompt a menu for user to choose a upload action
    void uploadFileOrMkdir();

    // prompt a dialog for user to choose whether upload or update
    void uploadOrUpdateFile(const QString& path);
    void uploadOrUpdateMutipleFile(const QStringList& paths);

    void onNavigatorClick(int id);

    void onGetDirentRename(const SeafDirent& dirent, QString new_name = QString());
    void onGetDirentRemove(const SeafDirent& dirent);
    void onGetDirentRemove(const QList<const SeafDirent*> &dirents);
    void onGetDirentShare(const SeafDirent& dirent);
    void onGetDirentUpdate(const SeafDirent& dirent);
    void onGetDirentsPaste();
    void onGetSyncSubdirectory(const QString &folder_name);
    void onCancelDownload(const SeafDirent& dirent);

    void onDirectoryCreateSuccess(const QString& path);
    void onDirectoryCreateFailed(const ApiError& error);
    void onDirentRenameSuccess(const QString& path, const QString& new_name);
    void onDirentRenameFailed(const ApiError& error);
    void onDirentRemoveSuccess(const QString& path);
    void onDirentRemoveFailed(const ApiError& error);
    void onDirentShareSuccess(const QString& link);
    void onDirentShareFailed(const ApiError& error);

    void onDirentsCopySuccess();
    void onDirentsCopyFailed(const ApiError& error);
    void onDirentsMoveSuccess();
    void onDirentsMoveFailed(const ApiError& error);

    void onCreateSubrepoSuccess(const ServerRepo& repo);
    void onCreateSubrepoFailed(const ApiError& error);

    void onFileAutoUpdated(const QString& repo_id, const QString& path);

private:
    Q_DISABLE_COPY(FileBrowserDialog)

    void closeEvent(QCloseEvent *event);
    void reject();
    bool hasFilesToBePasted();
    void setFilesToBePasted(bool is_copy, const QStringList &file_names);

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
                    bool overwrite = false);
    void uploadMultipleFile(const QStringList& paths, bool overwrite = false);

    void onDirClicked(const SeafDirent& dirent);
    void onFileClicked(const SeafDirent& dirent);

    void fetchDirents(bool force_refresh);

    bool setPasswordAndRetry(FileNetworkTask *task);

    const Account account_;
    const ServerRepo repo_;
    // current path
    QString current_path_;
    QStringList current_lpath_;
    QStack<QString> forward_history_;
    QStack<QString> backward_history_;
    static QStringList file_names_to_be_pasted_;
    static QString dir_path_to_be_pasted_from_;
    static QString repo_id_to_be_pasted_from_;
    static Account account_to_be_pasted_from_;
    static bool is_copyed_when_pasted_;

    QToolBar *toolbar_;
    QAction *backward_action_;
    QAction *forward_action_;
    QButtonGroup *path_navigator_;
    QList<QLabel*> path_navigator_separators_;
    QAction *gohome_action_;
    QAction *refresh_action_;

    QToolBar *status_bar_;
    QToolButton *upload_button_;
    QMenu *upload_menu_;
    QAction *upload_file_action_;
    QAction *upload_directory_action_;
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
