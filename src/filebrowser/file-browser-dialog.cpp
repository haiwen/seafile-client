#include <QtGlobal>
#include <QtWidgets>
#include <QDesktopServices>

#include "seafile-applet.h"
#include "account-mgr.h"
#include "utils/utils.h"
#include "utils/paint-utils.h"
#include "utils/file-utils.h"
#include "api/api-error.h"
#include "file-table.h"
#include "seaf-dirent.h"
#include "ui/loading-view.h"
#include "data-mgr.h"
#include "progress-dialog.h"
#include "tasks.h"
#include "ui/set-repo-password-dialog.h"
#include "auto-update-mgr.h"
#include "transfer-mgr.h"
#include "repo-service.h"
#include "rpc/local-repo.h"
#include "rpc/rpc-client.h"
#include "ui/private-share-dialog.h"
#include "shared-link-mgr.h"
#include "api/requests.h"

#include "file-browser-manager.h"
#include "file-browser-dialog.h"

#include "ui/download-repo-dialog.h"

#include "QtAwesome.h"

namespace {

enum {
    INDEX_LOADING_VIEW = 0,
    INDEX_TABLE_VIEW,
    INDEX_LOADING_FAILED_VIEW,
    INDEX_EMPTY_VIEW
};

const char *kLoadingFailedLabelName = "LoadingFailedText";
const int kToolBarIconSize = 20;
const int kStatusBarIconSize = 24;
//const int kStatusCodePasswordNeeded = 400;

void openFile(const QString& path)
{
    ::openInNativeExtension(path) || ::showInGraphicalShell(path);
#ifdef Q_OS_MAC
    MacImageFilesWorkAround::instance()->fileOpened(path);
#endif
}

bool inline findConflict(const QString &name, const QList<SeafDirent> &dirents) {
    bool found_conflict = false;
    // find if there exist a file with the same name
    Q_FOREACH(const SeafDirent &dirent, dirents)
    {
        if (dirent.name == name) {
            found_conflict = true;
            break;
        }
    }
    return found_conflict;
}

} // namespace

QStringList FileBrowserDialog::file_names_to_be_pasted_;
QString FileBrowserDialog::dir_path_to_be_pasted_from_;
QString FileBrowserDialog::repo_id_to_be_pasted_from_;
Account FileBrowserDialog::account_to_be_pasted_from_;
bool FileBrowserDialog::is_copyed_when_pasted_;

FileBrowserDialog::FileBrowserDialog(const Account &account, const ServerRepo& repo, const QString &path, QWidget *parent)
    : QDialog(parent),
      account_(account),
      repo_(repo),
      current_path_(path),
      current_readonly_(repo_.readonly)
{
    current_lpath_ = current_path_.split('/');

    data_mgr_ = new DataManager(account_);

    // In German translation there is a "seafile" string, so need to use tr("..").arg(..) here
    QString title = tr("Cloud File Browser");
    if (title.contains("%")) {
        title = title.arg(getBrand());
    }
    setWindowTitle(title);
    setWindowIcon(QIcon(":/images/seafile.png"));

    Qt::WindowFlags flags =
        (windowFlags() & ~Qt::WindowContextHelpButtonHint & ~Qt::Dialog) |
        Qt::Window | Qt::WindowSystemMenuHint | Qt::CustomizeWindowHint |
        Qt::WindowMinimizeButtonHint | Qt::WindowCloseButtonHint |
        Qt::WindowMaximizeButtonHint;

    setWindowFlags(flags);

    // setAttribute(Qt::WA_TranslucentBackground, true);

    createToolBar();
    createStatusBar();
    createLoadingFailedView();
    createEmptyView();
    createFileTable();

    QWidget* widget = new QWidget;
    widget->setObjectName("mainWidget");
    QVBoxLayout* layout = new QVBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    setLayout(layout);
    layout->addWidget(widget);

    QVBoxLayout *vlayout = new QVBoxLayout;
    vlayout->setContentsMargins(1, 0, 1, 0);
    vlayout->setSpacing(0);
    widget->setLayout(vlayout);

    stack_ = new QStackedWidget;
    stack_->insertWidget(INDEX_LOADING_VIEW, loading_view_);
    stack_->insertWidget(INDEX_TABLE_VIEW, table_view_);
    stack_->insertWidget(INDEX_LOADING_FAILED_VIEW, loading_failed_view_);
    stack_->insertWidget(INDEX_EMPTY_VIEW, empty_view_);
    stack_->setContentsMargins(0, 0, 0, 0);

    vlayout->addWidget(toolbar_);
    vlayout->addWidget(stack_);
    vlayout->addWidget(status_bar_);

    // this <--> table_view_
    connect(table_view_, SIGNAL(direntClicked(const SeafDirent&)),
            this, SLOT(onDirentClicked(const SeafDirent&)));
    connect(table_view_, SIGNAL(direntSaveAs(const QList<const SeafDirent*>&)),
            this, SLOT(onDirentSaveAs(const QList<const SeafDirent*>&)));
    connect(table_view_, SIGNAL(direntLock(const SeafDirent&)),
            this, SLOT(onGetDirentLock(const SeafDirent&)));
    connect(table_view_, SIGNAL(direntRename(const SeafDirent&)),
            this, SLOT(onGetDirentRename(const SeafDirent&)));
    connect(table_view_, SIGNAL(direntRemove(const SeafDirent&)),
            this, SLOT(onGetDirentRemove(const SeafDirent&)));
    connect(table_view_, SIGNAL(direntRemove(const QList<const SeafDirent*> &)),
            this, SLOT(onGetDirentRemove(const QList<const SeafDirent*> &)));
    connect(table_view_, SIGNAL(direntShare(const SeafDirent&)),
            this, SLOT(onGetDirentShare(const SeafDirent&)));
    connect(table_view_, SIGNAL(direntAdvancedShare(const SeafDirent&)),
            this, SLOT(onGetDirentAdvancedShare(const SeafDirent&)));
    connect(table_view_, SIGNAL(direntShareToUserOrGroup(const SeafDirent&, bool)),
            this, SLOT(onGetDirentShareToUserOrGroup(const SeafDirent&, bool)));
    connect(table_view_, SIGNAL(direntShareSeafile(const SeafDirent&)),
            this, SLOT(onGetDirentShareSeafile(const SeafDirent&)));
    connect(table_view_, SIGNAL(direntUpdate(const SeafDirent&)),
            this, SLOT(onGetDirentUpdate(const SeafDirent&)));
    connect(table_view_, SIGNAL(direntPaste()),
            this, SLOT(onGetDirentsPaste()));
    connect(table_view_, SIGNAL(cancelDownload(const SeafDirent&)),
            this, SLOT(onCancelDownload(const SeafDirent&)));
    connect(table_view_, SIGNAL(syncSubdirectory(const QString&)),
            this, SLOT(onGetSyncSubdirectory(const QString &)));

    //dirents <--> data_mgr_
    connect(data_mgr_, SIGNAL(getDirentsSuccess(bool, const QList<SeafDirent>&)),
            this, SLOT(onGetDirentsSuccess(bool, const QList<SeafDirent>&)));
    connect(data_mgr_, SIGNAL(getDirentsFailed(const ApiError&)),
            this, SLOT(onGetDirentsFailed(const ApiError&)));

    //create <--> data_mgr_
    connect(data_mgr_, SIGNAL(createDirectorySuccess(const QString&)),
            this, SLOT(onDirectoryCreateSuccess(const QString&)));
    connect(data_mgr_, SIGNAL(createDirectoryFailed(const ApiError&)),
            this, SLOT(onDirectoryCreateFailed(const ApiError&)));

    //lock <--> data_mgr_
    connect(data_mgr_, SIGNAL(lockFileSuccess(const QString&, bool)),
            this, SLOT(onFileLockSuccess(const QString&, bool)));
    connect(data_mgr_, SIGNAL(lockFileFailed(const ApiError&)),
            this, SLOT(onFileLockFailed(const ApiError&)));

    //rename <--> data_mgr_
    connect(data_mgr_, SIGNAL(renameDirentSuccess(const QString&, const QString&)),
            this, SLOT(onDirentRenameSuccess(const QString&, const QString&)));
    connect(data_mgr_, SIGNAL(renameDirentFailed(const ApiError&)),
            this, SLOT(onDirentRenameFailed(const ApiError&)));

    //remove <--> data_mgr_
    connect(data_mgr_, SIGNAL(removeDirentSuccess(const QString&)),
            this, SLOT(onDirentRemoveSuccess(const QString&)));
    connect(data_mgr_, SIGNAL(removeDirentFailed(const ApiError&)),
            this, SLOT(onDirentRemoveFailed(const ApiError&)));

    //copy <--> data_mgr_
    connect(data_mgr_, SIGNAL(copyDirentsSuccess()),
            this, SLOT(onDirentsCopySuccess()));
    connect(data_mgr_, SIGNAL(copyDirentsFailed(const ApiError&)),
            this, SLOT(onDirentsCopyFailed(const ApiError&)));

    //move <--> data_mgr_
    connect(data_mgr_, SIGNAL(moveDirentsSuccess()),
            this, SLOT(onDirentsMoveSuccess()));
    connect(data_mgr_, SIGNAL(moveDirentsFailed(const ApiError&)),
            this, SLOT(onDirentsMoveFailed(const ApiError&)));

    //subrepo <-->data_mgr_
    connect(data_mgr_, SIGNAL(createSubrepoSuccess(const ServerRepo &)),
            this, SLOT(onCreateSubrepoSuccess(const ServerRepo &)));
    connect(data_mgr_, SIGNAL(createSubrepoFailed(const ApiError&)),
            this, SLOT(onCreateSubrepoFailed(const ApiError&)));

    connect(AutoUpdateManager::instance(), SIGNAL(fileUpdated(const QString&, const QString&)),
            this, SLOT(onFileAutoUpdated(const QString&, const QString&)));

    QTimer::singleShot(0, this, SLOT(init()));
}

FileBrowserDialog::~FileBrowserDialog()
{
    delete data_mgr_;
}

void FileBrowserDialog::init()
{
    enterPath(current_path_);
}

void FileBrowserDialog::createToolBar()
{
    toolbar_ = new QToolBar;
    toolbar_->setObjectName("topBar");
    toolbar_->setIconSize(QSize(kToolBarIconSize, kToolBarIconSize));

    backward_button_ = new QToolButton(this);
    backward_button_->setText(tr("Back"));
    backward_button_->setObjectName("backwardButton");
    backward_button_->setIcon(QIcon(":/images/filebrowser/backward.png"));
    backward_button_->setEnabled(false);
    backward_button_->setShortcut(QKeySequence::Back);
    toolbar_->addWidget(backward_button_);
    connect(backward_button_, SIGNAL(clicked()), this, SLOT(goBackward()));

    forward_button_ = new QToolButton(this);
    forward_button_->setText(tr("Forward"));
    forward_button_->setObjectName("forwardButton");
    forward_button_->setIcon(QIcon(":/images/filebrowser/forward.png"));
    forward_button_->setEnabled(false);
    forward_button_->setShortcut(QKeySequence::Forward);
    toolbar_->addWidget(forward_button_);
    connect(forward_button_, SIGNAL(clicked()), this, SLOT(goForward()));

    gohome_action_ = new QAction(tr("Home"), this);
    gohome_action_->setIcon(QIcon(":images/filebrowser/home.png"));
    connect(gohome_action_, SIGNAL(triggered()), this, SLOT(goHome()));
    toolbar_->addAction(gohome_action_);

    path_navigator_ = new QButtonGroup(this);
    connect(path_navigator_, SIGNAL(buttonClicked(int)),
            this, SLOT(onNavigatorClick(int)));

}

void FileBrowserDialog::createStatusBar()
{
    status_bar_ = new QToolBar;
    status_bar_->setObjectName("statusBar");
    status_bar_->setIconSize(QSize(kStatusBarIconSize, kStatusBarIconSize));

    QWidget *spacer1 = new QWidget;
    spacer1->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
    status_bar_->addWidget(spacer1);

    // Submenu
    upload_menu_ = new QMenu(status_bar_);

    // Submenu's Action 1: Upload File
    upload_file_action_ = new QAction(tr("Upload files"), upload_menu_);
    connect(upload_file_action_, SIGNAL(triggered()), this, SLOT(chooseFileToUpload()));
    upload_menu_->addAction(upload_file_action_);

    // Submenu's Action 2: Upload directory (only pro version's server supports it)
    if (account_.isPro()) {
        upload_directory_action_ = new QAction(tr("Upload a directory"), upload_menu_);
        connect(upload_directory_action_, SIGNAL(triggered()), this, SLOT(chooseDirectoryToUpload()));
        upload_menu_->addAction(upload_directory_action_);
    } else {
        upload_directory_action_ = NULL;
    }

    // Submenu's Action 3: Make a new directory
    mkdir_action_ = new QAction(tr("Create a folder"), upload_menu_);
    connect(mkdir_action_, SIGNAL(triggered()), this, SLOT(onMkdirButtonClicked()));
    upload_menu_->addAction(mkdir_action_);

    // Action to trigger Submenu
    upload_button_ = new QToolButton;
    upload_button_->setObjectName("uploadButton");
    upload_button_->setToolTip(tr("Upload files"));
    upload_button_->setIcon(QIcon(":/images/toolbar/add.png"));
    connect(upload_button_, SIGNAL(clicked()), this, SLOT(uploadFileOrMkdir()));
    status_bar_->addWidget(upload_button_);

    if (current_readonly_) {
        upload_button_->setEnabled(false);
        upload_button_->setToolTip(tr("You don't have permission to upload files to this library"));
    }

    details_label_ = new QLabel;
    details_label_->setAlignment(Qt::AlignCenter | Qt::AlignVCenter);
    details_label_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    status_bar_->addWidget(details_label_);

    refresh_action_ = new QAction(this);
    refresh_action_->setIcon(QIcon(":/images/toolbar/refresh.png"));
    refresh_action_->setToolTip(tr("Refresh"));
    connect(refresh_action_, SIGNAL(triggered()), this, SLOT(forceRefresh()));
    refresh_action_->setShortcut(QKeySequence::Refresh);
    status_bar_->addAction(refresh_action_);

    QWidget *spacer2 = new QWidget;
    spacer2->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
    status_bar_->addWidget(spacer2);
}

void FileBrowserDialog::createFileTable()
{
    loading_view_ = new LoadingView;
    table_view_ = new FileTableView(this);
    table_model_ = new FileTableModel(this);
    table_view_->setModel(table_model_);

    connect(table_view_, SIGNAL(dropFile(const QStringList&)),
            this, SLOT(uploadOrUpdateMutipleFile(const QStringList&)));
}

void FileBrowserDialog::forceRefresh()
{
    fetchDirents(true);
}

void FileBrowserDialog::fetchDirents()
{
    fetchDirents(false);
}

void FileBrowserDialog::fetchDirents(bool force_refresh)
{
    if (repo_.encrypted && !data_mgr_->isRepoPasswordSet(repo_.id)) {
        SetRepoPasswordDialog password_dialog(repo_, this);
        if (password_dialog.exec() != QDialog::Accepted) {
            reject();
            return;
        } else {
            data_mgr_->setRepoPasswordSet(repo_.id, password_dialog.password());
        }
    }

    if (!force_refresh) {
        QList<SeafDirent> dirents;
        if (data_mgr_->getDirents(repo_.id, current_path_, &dirents, &current_readonly_)) {
            updateTable(dirents);
            return;
        }
    }

    showLoading();
    data_mgr_->getDirentsFromServer(repo_.id, current_path_);
}

void FileBrowserDialog::onGetDirentsSuccess(bool current_readonly, const QList<SeafDirent>& dirents)
{
    current_readonly_ = current_readonly;
    updateTable(dirents);
}

void FileBrowserDialog::onGetDirentsFailed(const ApiError& error)
{
    stack_->setCurrentIndex(INDEX_LOADING_FAILED_VIEW);
}

void FileBrowserDialog::onMkdirButtonClicked()
{
    QString name = seafApplet->getText(this, tr("Create a folder"),
        tr("Folder name"));
    name = name.trimmed();

    if (name.isEmpty())
        return;

    // invalid name
    if (name.contains("/")) {
        seafApplet->warningBox(tr("Invalid folder name!"), this);
        return;
    }

    if (findConflict(name, table_model_->dirents())) {
        seafApplet->warningBox(
            tr("The name \"%1\" is already taken.").arg(name), this);
        return;
    }

    createDirectory(name);
}

void FileBrowserDialog::createLoadingFailedView()
{
    loading_failed_view_ = new QLabel;
    loading_failed_view_->setObjectName(kLoadingFailedLabelName);
    QString link = QString("<a style=\"color:#777\" href=\"#\">%1</a>").arg(tr("retry"));
    QString label_text = tr("Failed to get files information<br/>"
                            "Please %1").arg(link);
    loading_failed_view_->setText(label_text);
    loading_failed_view_->setAlignment(Qt::AlignCenter);

    connect(loading_failed_view_, SIGNAL(linkActivated(const QString&)),
            this, SLOT(forceRefresh()));
}

void FileBrowserDialog::createEmptyView()
{
    empty_view_ = new QLabel;
    empty_view_->setText(tr("This folder is empty."));
    empty_view_->setAlignment(Qt::AlignCenter);
}

void FileBrowserDialog::onDirentClicked(const SeafDirent& dirent)
{
    if (dirent.isDir()) {
        onDirClicked(dirent);
    } else {
        onFileClicked(dirent);
    }
}

void FileBrowserDialog::onDirentSaveAs(const QList<const SeafDirent*>& dirents)
{
    static QDir download_dir(defaultDownloadDir());
    if (dirents.isEmpty())
        return;
    if (!download_dir.exists())
        download_dir = QDir::home();

    // handle when we have only one file
    if (dirents.size() == 1) {
        const SeafDirent &dirent = *dirents.front();
        QString local_path = QFileDialog::getSaveFileName(this, tr("Enter name of file to save to..."), download_dir.filePath(dirent.name));
        if (local_path.isEmpty())
            return;
        download_dir = QFileInfo(local_path).dir();
        if (QFileInfo(local_path).exists()) {
          // it is asked by the QFileDialog::getSaveFileName if overwrite the file
          if (!QFile::remove(local_path)) {
              seafApplet->warningBox(tr("Unable to remove file \"%1\""), this);
              return;
          }
        }
        FileDownloadTask *task = data_mgr_->createSaveAsTask(repo_.id, ::pathJoin(current_path_, dirent.name), local_path);
        connect(task, SIGNAL(finished(bool)), this, SLOT(onDownloadFinished(bool)));
        return;
    }

    QString local_dir = QFileDialog::getExistingDirectory(this, tr("Enter the path of the folder you want to save to..."), download_dir.path());
    if (local_dir.isEmpty())
        return;
    download_dir = local_dir;
    //
    // scan for existing files and folders
    // then begin downloading
    //
    Q_FOREACH(const SeafDirent* dirent, dirents) {
        QString local_path = ::pathJoin(local_dir, dirent->name);
        if  (QFileInfo(local_path).exists()) {
              if (!seafApplet->yesOrNoBox(tr("Do you want to overwrite the existing file \"%1\"?").arg(dirent->name))) {
                  return;
              }
              if (!QFile::remove(local_path)) {
                  seafApplet->warningBox(tr("Unable to remove file \"%1\""), this);
                  return;
              }
        }
        // get them to be downloaded
        FileDownloadTask *task = data_mgr_->createSaveAsTask(repo_.id, ::pathJoin(current_path_, dirent->name), local_path);
        connect(task, SIGNAL(finished(bool)), this, SLOT(onDownloadFinished(bool)));
    }
}

void FileBrowserDialog::showLoading()
{
    forward_button_->setEnabled(false);
    backward_button_->setEnabled(false);
    upload_button_->setEnabled(false);
    gohome_action_->setEnabled(false);
    stack_->setCurrentIndex(INDEX_LOADING_VIEW);
}

void FileBrowserDialog::onDirClicked(const SeafDirent& dir)
{
    const QString& path = ::pathJoin(current_path_, dir.name);
    backward_history_.push(current_path_);
    forward_history_.clear();
    enterPath(path);
}

void FileBrowserDialog::enterPath(const QString& path)
{
    current_path_ = path;
    // use QUrl::toPercentEncoding if need
    fetchDirents();

    // current_path should be guaranteed safe to split!
    current_lpath_ = current_path_.split('/');
    // if the last element is empty (i.e. current_path_ ends with an extra "/"),
    // remove it
    if(current_lpath_.last().isEmpty())
        current_lpath_.pop_back();

    // remove all old buttons for navigator except the root
    QList<QAbstractButton *> buttons = path_navigator_->buttons();
    Q_FOREACH(QAbstractButton *button, buttons)
    {
        path_navigator_->removeButton(button);
        delete button;
    }
    Q_FOREACH(QLabel *label, path_navigator_separators_)
    {
        delete label;
    }
    path_navigator_separators_.clear();

    gohome_action_->setEnabled(path != "/");

    // root is special
    if (path == "/") {
        QLabel *root = new QLabel(repo_.name);
        toolbar_->addWidget(root);
        path_navigator_separators_.push_back(root);
    } else {
        QPushButton *root = new QPushButton(repo_.name);
        root->setObjectName("homeButton");
        root->setFlat(true);
        root->setCursor(Qt::PointingHandCursor);
        toolbar_->addWidget(root);
        path_navigator_->addButton(root, 0);

        // add new buttons for navigator except the root
        for(int i = 1; i < current_lpath_.size(); i++) {
            QLabel *separator = new QLabel("/");
            separator->setBaseSize(4, 7);
            separator->setPixmap(QIcon(":/images/filebrowser/path-separator.png").pixmap(QSize(4, 7)));
            path_navigator_separators_.push_back(separator);
            toolbar_->addWidget(separator);
            if (i != current_lpath_.size() - 1) {
                QPushButton* button = new QPushButton(current_lpath_[i]);
                button->setFlat(true);
                button->setCursor(Qt::PointingHandCursor);
                path_navigator_->addButton(button, i);
                toolbar_->addWidget(button);
            } else {
                QLabel *separator = new QLabel(current_lpath_[i]);
                toolbar_->addWidget(separator);
                path_navigator_separators_.push_back(separator);
            }
        }
    }
}

void FileBrowserDialog::onFileClicked(const SeafDirent& file)
{
    QString fpath = ::pathJoin(current_path_, file.name);

    LocalRepo repo;
    if (seafApplet->rpcClient()->getLocalRepo(repo_.id, &repo) == 0) {
        QString synced_path = ::pathJoin(repo.worktree, fpath);
        if (!QFileInfo(synced_path).exists()) {
            seafApplet->warningBox(tr("File \"%1\" haven't been synced").arg(file.name));
            return;
        }
        openFile(synced_path);
        return;
    }
    QString cached_file = data_mgr_->getLocalCachedFile(repo_.id, fpath, file.id);
    if (!cached_file.isEmpty() && QFileInfo(cached_file).exists()) {
        // double-checked the watch, since it might fails sometime
        AutoUpdateManager::instance()->watchCachedFile(account_, repo_.id, fpath);
        openFile(cached_file);
        return;
    } else {
        if (TransferManager::instance()->getDownloadTask(repo_.id, fpath)) {
            return;
        }
        AutoUpdateManager::instance()->removeWatch(
            DataManager::getLocalCacheFilePath(repo_.id, fpath));
        downloadFile(fpath);
    }
}

void FileBrowserDialog::createDirectory(const QString &name)
{
    data_mgr_->createDirectory(repo_.id, ::pathJoin(current_path_, name));
}

void FileBrowserDialog::downloadFile(const QString& path)
{
    FileDownloadTask *task = data_mgr_->createDownloadTask(repo_.id, path);
    connect(task, SIGNAL(finished(bool)), this, SLOT(onDownloadFinished(bool)));
}

void FileBrowserDialog::uploadFile(const QString& path, const QString& name,
                                   bool overwrite)
{
    if (QFileInfo(path).isDir() && !account_.isPro()) {
        seafApplet->warningBox(tr("Feature not supported"), this);
        return;
    }

    FileUploadTask *task =
      data_mgr_->createUploadTask(repo_.id, current_path_, path, name, overwrite);
    connect(task, SIGNAL(finished(bool)), this, SLOT(onUploadFinished(bool)));
    FileBrowserProgressDialog *dialog = new FileBrowserProgressDialog(task, this);
    task->start();

    // set dialog attributes
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowModality(Qt::WindowModal);
    dialog->show();
    dialog->raise();
    dialog->activateWindow();
}

void FileBrowserDialog::uploadMultipleFile(const QStringList& names,
                                           bool overwrite)
{
    if (names.empty())
        return;
    const QString path = QFileInfo(names.front()).path();
    QStringList fnames;
    Q_FOREACH(const QString &name, names) {
        const QFileInfo file = name;
        if (file.isDir()) {
            uploadOrUpdateFile(name);
        } else if (file.path() == path) {
            fnames.push_back(file.fileName());
        }
    }

    FileUploadTask *task =
      data_mgr_->createUploadMultipleTask(repo_.id, current_path_, path, fnames,
                                          overwrite);
    connect(task, SIGNAL(finished(bool)), this, SLOT(onUploadFinished(bool)));
    FileBrowserProgressDialog *dialog = new FileBrowserProgressDialog(task, this);
    task->start();

    // set dialog attributes
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowModality(Qt::WindowModal);
    dialog->show();
    dialog->raise();
    dialog->activateWindow();
}

void FileBrowserDialog::uploadFileOrMkdir()
{
    // the menu shows in the right-down corner
    upload_menu_->exec(upload_button_->mapToGlobal(
        QPoint(upload_button_->width()-2, upload_button_->height()/2)));
}

void FileBrowserDialog::uploadOrUpdateFile(const QString& path)
{
    const QString name = ::getBaseName(path);

    // ignore the confirm procedure for uploading directory, which is non-sense
    if (QFileInfo(path).isDir())
        return uploadFile(path, name);

    // prompt a dialog to confirm to overwrite the current file
    if (findConflict(name, table_model_->dirents())) {
        QMessageBox::StandardButton ret = seafApplet->yesNoCancelBox(
            tr("File %1 already exists.<br/>"
               "Do you like to overwrite it?<br/>"
               "<small>(Choose No to upload using an alternative name).</small>").arg(name),
            this,
            QMessageBox::Cancel);

        if (ret == QMessageBox::Cancel) {
            return;
        } else if (ret == QMessageBox::Yes) {
            // overwrite the file
            uploadFile(path, name, true);
            return;
        }
    }

    // in other cases, use upload
    uploadFile(path, name);
}

void FileBrowserDialog::uploadOrUpdateMutipleFile(const QStringList &paths)
{
    if (paths.size() == 1)
        uploadOrUpdateFile(paths.front());
    else
        uploadMultipleFile(paths);
}

void FileBrowserDialog::onDownloadFinished(bool success)
{
    FileDownloadTask *task = qobject_cast<FileDownloadTask *>(sender());
    if (task == NULL)
        return;
    if (success) {
        if (!task->isSaveAsTask())
            openFile(task->localFilePath());
    } else {
        if (repo_.encrypted &&
            setPasswordAndRetry(task)) {
            return;
        }

        if (task->error() == FileNetworkTask::TaskCanceled)
            return;

        QString msg = tr("Failed to download file: %1").arg(task->errorString());
        seafApplet->warningBox(msg, this);
    }
}

void FileBrowserDialog::onUploadFinished(bool success)
{
    FileUploadTask *task = qobject_cast<FileUploadTask *>(sender());
    if (task == NULL)
        return;

    if (!success) {
        if (repo_.encrypted &&
            setPasswordAndRetry(task)) {
            return;
        }

        // always force a refresh for uploading directory
        if (qobject_cast<FileUploadDirectoryTask*>(sender()))
            forceRefresh();

        if (task->error() == FileNetworkTask::TaskCanceled)
            return;

        QString msg = tr("Failed to upload file: %1").arg(task->errorString());
        seafApplet->warningBox(msg, this);
        return;
    }

    QString local_path = task->localFilePath();
    QStringList names;

    // Upload Directory Task
    if (qobject_cast<FileUploadDirectoryTask *>(sender())) {
        const SeafDirent dir = {
          SeafDirent::DIR,
          false,
          "",
          task->name(),
          0,
          QDateTime::currentDateTime().toTime_t(),
          false,
          false,
          "",
          0
        };
        // TODO: insert the Item prior to the item where uploading occurs
        table_model_->insertItem(0, dir);
        return;
    }

    // Upload Multiple Task
    FileUploadMultipleTask *multi_task = qobject_cast<FileUploadMultipleTask *>(sender());

    if (multi_task == NULL) {
      names.push_back(task->name());
      local_path = QFileInfo(local_path).absolutePath();
    } else {
      names = multi_task->names();
      local_path = QFileInfo(local_path).absoluteFilePath();
    }

    // require a forceRefresh if conflicting filename found
    Q_FOREACH(const QString &name, names)
    {
        if (findConflict(name, table_model_->dirents())) {
            forceRefresh();
            return;
        }
    }

    // add the items to tableview
    Q_FOREACH(const QString &name, names) {
        const QFileInfo file = QDir(local_path).filePath(name);
        const SeafDirent dirent = {
          SeafDirent::FILE,
          false,
          "",
          name,
          static_cast<quint64>(file.size()),
          QDateTime::currentDateTime().toTime_t(),
          false,
          false,
          "",
          0
        };
        if (task->useUpload())
            table_model_->appendItem(dirent);
        else
            table_model_->replaceItem(name, dirent);
    }
}

bool FileBrowserDialog::setPasswordAndRetry(FileNetworkTask *task)
{
    if (task->httpErrorCode() == 400) {
        SetRepoPasswordDialog password_dialog(repo_, this);
        if (password_dialog.exec() == QDialog::Accepted) {
            if (task->type() == FileNetworkTask::Download)
                downloadFile(task->path());
            else
                uploadOrUpdateFile(task->path());
            return true;
        }
    }

    return false;
}

void FileBrowserDialog::goForward()
{
    QString path;
    if (!forward_history_.empty()) {
        path = forward_history_.pop();
    }
    backward_history_.push(current_path_);
    enterPath(path);
}

void FileBrowserDialog::goBackward()
{
    QString path;
    if (!backward_history_.empty()) {
        path = backward_history_.pop();
    }
    forward_history_.push(current_path_);
    enterPath(path);
}

void FileBrowserDialog::goHome()
{
    if (current_path_ == "/") {
        return;
    }
    backward_history_.push(current_path_);
    forward_history_.clear();
    enterPath("/");
}

void FileBrowserDialog::updateTable(const QList<SeafDirent>& dirents)
{
    // Commented out because the empty view can't handle file drag & drop events.

    // if (dirents.isEmpty()) {
    //     stack_->setCurrentIndex(INDEX_EMPTY_VIEW);
    // } else {
    //     table_model_->setDirents(dirents);
    //     stack_->setCurrentIndex(INDEX_TABLE_VIEW);
    // }

    table_model_->setDirents(dirents);
    stack_->setCurrentIndex(INDEX_TABLE_VIEW);

    if (!forward_history_.empty()) {
        forward_button_->setEnabled(true);
    } else {
        forward_button_->setEnabled(false);
    }
    if (!backward_history_.empty()) {
        backward_button_->setEnabled(true);
    } else {
        backward_button_->setEnabled(false);
    }
    if (!current_readonly_) {
        upload_button_->setEnabled(true);
    }
    gohome_action_->setEnabled(current_path_ != "/");
}

void FileBrowserDialog::chooseFileToUpload()
{
    QStringList paths = QFileDialog::getOpenFileNames(this, tr("Select a file to upload"), QDir::homePath());
    if (paths.empty())
        return;
    uploadOrUpdateMutipleFile(paths);
}

void FileBrowserDialog::chooseDirectoryToUpload()
{
    QString path = QFileDialog::getExistingDirectory(this, tr("Select a directory to upload"), QDir::homePath());
    if (path.isEmpty())
        return;
    uploadOrUpdateFile(path);
}

void FileBrowserDialog::onNavigatorClick(int id)
{
    // calculate the path
    QString path = "/";
    for(int i = 1; i <= id; i++)
      path += current_lpath_[i] + "/";

    backward_history_.push(current_path_);
    forward_history_.clear();
    enterPath(path);
}

void FileBrowserDialog::onGetDirentLock(const SeafDirent& dirent)
{
    data_mgr_->lockFile(repo_.id, ::pathJoin(current_path_, dirent.name), !dirent.is_locked);
}

void FileBrowserDialog::onGetDirentRename(const SeafDirent& dirent,
                                          QString new_name)
{
    if (new_name.isEmpty()) {
        new_name = seafApplet->getText(this, tr("Rename"),
                   QObject::tr("Rename %1 to").arg(dirent.name),
                   QLineEdit::Normal,
                   dirent.name);
        // trim the whites
        new_name = new_name.trimmed();
        // if cancelled or empty
        if (new_name.isEmpty())
            return;
        // if no change
        if (dirent.name == new_name)
            return;
    }
    data_mgr_->renameDirent(repo_.id,
                            ::pathJoin(current_path_, dirent.name),
                            new_name,
                            dirent.isFile());
}

void FileBrowserDialog::onGetDirentRemove(const SeafDirent& dirent)
{
    // if (seafApplet->yesOrNoBox(dirent.isFile() ?
    //         tr("Do you really want to delete file \"%1\"?").arg(dirent.name) :
    //         tr("Do you really want to delete folder \"%1\"?").arg(dirent.name), this, false))
    data_mgr_->removeDirent(repo_.id, pathJoin(current_path_, dirent.name),
                            dirent.isFile());
}

void FileBrowserDialog::onGetDirentRemove(const QList<const SeafDirent*> &dirents)
{
    if (!seafApplet->yesOrNoBox(tr("Do you really want to delete these items"), this, false))
        return;

    Q_FOREACH(const SeafDirent* dirent, dirents)
    {
        data_mgr_->removeDirent(repo_.id, pathJoin(current_path_, dirent->name),
                                dirent->isFile());
    }
}

void FileBrowserDialog::onGetDirentShare(const SeafDirent& dirent)
{
    SharedLinkRequestParams params;
    params.account = account_;
    params.repo_id = repo_.id;
    params.path_in_repo = ::pathJoin(current_path_, dirent.name);
    params.is_file = dirent.isFile();
    params.internal = false;
    params.advanced = false;
    SharedLinkManager::instance()->generateShareLink(params);
}

void FileBrowserDialog::onGetDirentAdvancedShare(const SeafDirent& dirent)
{
    SharedLinkRequestParams params;
    params.account = account_;
    params.repo_id = repo_.id;
    params.path_in_repo = ::pathJoin(current_path_, dirent.name);
    params.is_file = dirent.isFile();
    params.internal = false;
    params.advanced = true;
    SharedLinkManager::instance()->generateShareLink(params);
}

void FileBrowserDialog::onGetDirentShareToUserOrGroup(const SeafDirent& dirent,
                                                bool to_group)
{
    PrivateShareDialog dialog(account_, repo_.id, repo_.name,
                              ::pathJoin(current_path_, dirent.name), to_group,
                              this);
    dialog.exec();
}

void FileBrowserDialog::onGetDirentShareSeafile(const SeafDirent& dirent)
{
    SharedLinkRequestParams params;
    params.account = account_;
    params.repo_id = repo_.id;
    params.path_in_repo = ::pathJoin(current_path_, dirent.name);
    params.is_file = dirent.isFile();
    params.internal = true;
    params.advanced = false;
    SharedLinkManager::instance()->generateShareLink(params);
}

void FileBrowserDialog::onDirectoryCreateSuccess(const QString &path)
{
    const QString name = QFileInfo(path).fileName();
    // if no longer current level
    if (::pathJoin(current_path_, name) != path)
        return;
    const SeafDirent dirent = {SeafDirent::DIR, false, "", name, 0,
                               QDateTime::currentDateTime().toTime_t(), false,
                               false, "", 0};
    // TODO insert to the pos where the drop event triggered
    table_model_->insertItem(0, dirent);
}

void FileBrowserDialog::onDirectoryCreateFailed(const ApiError&error)
{
    seafApplet->warningBox(tr("Create folder failed"), this);
}

void FileBrowserDialog::onFileLockSuccess(const QString& path, bool lock)
{
    const QString name = QFileInfo(path).fileName();
    // if no longer current level
    if (::pathJoin(current_path_, name) != path)
        return;
    const SeafDirent *old_dirent = NULL;
    Q_FOREACH(const SeafDirent& dirent, table_model_->dirents())
    {
        if (dirent.name == name) {
            old_dirent = &dirent;
            break;
        }
    }
    if (!old_dirent)
        return;
    SeafDirent new_dirent = *old_dirent;
    new_dirent.is_locked = new_dirent.locked_by_me = lock;
    new_dirent.lock_owner = account_.username;
    table_model_->replaceItem(name, new_dirent);
}

void FileBrowserDialog::onFileLockFailed(const ApiError& error)
{
    seafApplet->warningBox(tr("Lock file failed"), this);
}

void FileBrowserDialog::onDirentRenameSuccess(const QString& path,
                                              const QString& new_name)
{
    const QString name = QFileInfo(path).fileName();
    // if no longer current level
    if (::pathJoin(current_path_, name) != path)
        return;
    table_model_->renameItemNamed(name, new_name);
}

void FileBrowserDialog::onGetDirentUpdate(const SeafDirent& dirent)
{
    QString path = QFileDialog::getOpenFileName(this,
        tr("Select a file to update %1").arg(dirent.name), QDir::homePath());
    if (!path.isEmpty()) {
        uploadFile(path, dirent.name, true);
    }
}

void FileBrowserDialog::onDirentRenameFailed(const ApiError&error)
{
    seafApplet->warningBox(tr("Rename failed"), this);
}

void FileBrowserDialog::onDirentRemoveSuccess(const QString& path)
{
    const QString name = QFileInfo(path).fileName();
    // if no longer current level
    if (::pathJoin(current_path_, name) != path)
        return;
    table_model_->removeItemNamed(name);
}

void FileBrowserDialog::onDirentRemoveFailed(const ApiError&error)
{
    seafApplet->warningBox(tr("Remove failed"), this);
}

void FileBrowserDialog::onFileAutoUpdated(const QString& repo_id, const QString& path)
{
    if (repo_id == repo_.id && path == current_path_) {
        forceRefresh();
    }
}

void FileBrowserDialog::onCancelDownload(const SeafDirent& dirent)
{
    TransferManager::instance()->cancelDownload(repo_.id,
        ::pathJoin(current_path_, dirent.name));
}


void FileBrowserDialog::done(int retval)
{
    emit aboutToClose();
    QDialog::done(retval);
}

bool FileBrowserDialog::hasFilesToBePasted() {
    return !file_names_to_be_pasted_.empty();
}

void FileBrowserDialog::setFilesToBePasted(bool is_copy, const QStringList &file_names)
{
    is_copyed_when_pasted_ = is_copy;
    dir_path_to_be_pasted_from_ = current_path_;
    file_names_to_be_pasted_ = file_names;
    repo_id_to_be_pasted_from_ = repo_.id;
    account_to_be_pasted_from_ = account_;
}

void FileBrowserDialog::onGetDirentsPaste()
{
    if (repo_id_to_be_pasted_from_ == repo_.id &&
        current_path_ == dir_path_to_be_pasted_from_) {
        seafApplet->warningBox(tr("Cannot paste files from the same folder"), this);
        return;
    }
    if (is_copyed_when_pasted_)
        data_mgr_->copyDirents(repo_id_to_be_pasted_from_,
                               dir_path_to_be_pasted_from_,
                               file_names_to_be_pasted_,
                               repo_.id,
                               current_path_);
    else
        data_mgr_->moveDirents(repo_id_to_be_pasted_from_,
                               dir_path_to_be_pasted_from_,
                               file_names_to_be_pasted_,
                               repo_.id,
                               current_path_);
}

void FileBrowserDialog::onDirentsCopySuccess()
{
    forceRefresh();
}

void FileBrowserDialog::onDirentsCopyFailed(const ApiError& error)
{
    seafApplet->warningBox(tr("Copy failed"), this);
}

void FileBrowserDialog::onDirentsMoveSuccess()
{
    file_names_to_be_pasted_.clear();
    FileBrowserDialog *dialog =
        FileBrowserManager::getInstance()->getDialog(account_to_be_pasted_from_, repo_id_to_be_pasted_from_);
    if (dialog != NULL && dialog->current_path_ == dir_path_to_be_pasted_from_)
        dialog->forceRefresh();
    forceRefresh();
}

void FileBrowserDialog::onDirentsMoveFailed(const ApiError& error)
{
    seafApplet->warningBox(tr("Move failed"), this);
}

void FileBrowserDialog::onGetSyncSubdirectory(const QString &folder_name)
{
    data_mgr_->createSubrepo(folder_name, repo_.id, ::pathJoin(current_path_, folder_name));
}

void FileBrowserDialog::onCreateSubrepoSuccess(const ServerRepo &repo)
{
    // if we have not synced before
    bool has_local = false;
    if (seafApplet->rpcClient()->hasLocalRepo(repo.id)) {
        has_local = true;
    } else {
        // if we have not synced, do it
        DownloadRepoDialog dialog(account_, repo,
                                  repo.encrypted ? data_mgr_->repoPassword(repo_.id) : QString(), this);

        has_local = dialog.exec() == QDialog::Accepted;
    }

    if (has_local)
        RepoService::instance()->saveSyncedSubfolder(repo);
}

void FileBrowserDialog::onCreateSubrepoFailed(const ApiError&error)
{
    seafApplet->warningBox(tr("Create library failed!"), this);
}
