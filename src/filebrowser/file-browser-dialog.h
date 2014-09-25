#ifndef SEAFILE_CLIENT_FILE_BROWSER_DIALOG_H
#define SEAFILE_CLIENT_FILE_BROWSER_DIALOG_H

#include <QDialog>

class ServerRepo;
class QVBoxLayout;
class QToolBar;
class QLabel;
class QAction;
class QStackedWidget;
class QLineEdit;
class FileNetworkTask;
class FileBrowserProgressDialog;

class FileTableView;
class FileTableModel;
class SeafDirent;

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
{
    Q_OBJECT
public:
    FileBrowserDialog(const ServerRepo& repo, QWidget *parent=0);
    ~FileBrowserDialog();

private slots:
    void onLoading();
    void onLoadingFinished();
    void onLoadingFailed();

    void onBackwardEnabled(bool enabled);
    void onForwardEnabled(bool enabled);
    void onDownloadEnabled(bool enabled);

private:
    Q_DISABLE_COPY(FileBrowserDialog)

    void createToolBar();
    void createStatusBar();
    void createFileTable();
    void createLoadingFailedView();
    void createStackWidget();

    // template string
    QString repo_id_and_path_;

    QVBoxLayout *layout_;
    QToolBar *toolbar_;
    QAction *backward_action_;
    QAction *forward_action_;
    QLineEdit *path_line_edit_;
    QAction *navigate_home_action_;
    QToolBar *status_bar_;
    QAction *settings_action_;
    QAction *upload_action_;
    QAction *download_action_;
    QLabel *details_label_;
    QAction *refresh_action_;
    QAction *open_cache_dir_action_;

    QStackedWidget *stack_;
    QWidget *loading_view_;
    QWidget *loading_failed_view_;
    FileTableView *table_view_;
    FileTableModel *table_model_;
    FileBrowserProgressDialog *file_progress_dialog_;
};


#endif  // SEAFILE_CLIENT_FILE_BROWSER_DIALOG_H
