#ifndef SEAFILE_CLIENT_FILE_BROWSER_DIALOG_H
#define SEAFILE_CLIENT_FILE_BROWSER_DIALOG_H

#include <QDialog>
// #include "ui_file-browser-dialog.h"

#include "api/server-repo.h"

class QToolBar;
class QAction;
class QStackedWidget;

class ApiError;
class FileTableView;
class FileTableModel;
class SeafDirent;
class GetDirentsRequest;
class FileBrowserCache;
class DataManager;

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

private slots:
    void onGetDirentsSuccess(const std::vector<SeafDirent>& dirents);
    void onGetDirentsFailed(const ApiError& error);
    void fetchDirents();
    void onDirentClicked(const SeafDirent& dirent);

private:
    Q_DISABLE_COPY(FileBrowserDialog)

    void createToolBar();
    void createFileTable();
    void createLoadingFailedView();

    void onDirClicked(const SeafDirent& dirent);
    void onFileClicked(const SeafDirent& dirent);

    ServerRepo repo_;
    // current path
    QString path_;

    QToolBar *toolbar_;
    QAction *refresh_action_;

    QStackedWidget *stack_;
    QWidget *loading_view_;
    QWidget *loading_failed_view_;
    FileTableView *table_view_;
    FileTableModel *table_model_;

    DataManager *data_mgr_;
};


#endif  // SEAFILE_CLIENT_FILE_BROWSER_DIALOG_H
