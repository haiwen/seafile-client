#include <QtGui>
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
#include "data-mgr.h"
#include "progress-dialog.h"
#include "tasks.h"
#include "ui/set-repo-password-dialog.h"
#include "sharedlink-dialog.h"

#include "file-browser-dialog.h"

namespace {

enum {
    INDEX_LOADING_VIEW = 0,
    INDEX_TABLE_VIEW,
    INDEX_LOADING_FAILED_VIEW
};

const char *kLoadingFaieldLabelName = "loadingFailedText";
const int kToolBarIconSize = 20;
const int kStatusBarIconSize = 24;
//const int kStatusCodePasswordNeeded = 400;

void openFile(const QString& path)
{
    ::openInNativeExtension(path) || ::showInGraphicalShell(path);
}

} // namespace

FileBrowserDialog::FileBrowserDialog(const ServerRepo& repo, QWidget *parent)
    : QDialog(parent),
      repo_(repo)
{
    current_path_ = "/";
    // since root is special, the next step is unnecessary
    // current_lpath_.push_back("");

    const Account& account = seafApplet->accountManager()->currentAccount();
    data_mgr_ = new DataManager(account);

    setWindowTitle(tr("Cloud File Browser"));
    setWindowIcon(QIcon(":/images/seafile.png"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint & ~Qt::Dialog
                   | Qt::WindowMinimizeButtonHint | Qt::Window);

    createToolBar();
    createStatusBar();
    createLoadingFailedView();
    createFileTable();

    QVBoxLayout *vlayout = new QVBoxLayout;
    vlayout->setContentsMargins(0, 6, 0, 0);
    vlayout->setSpacing(0);
    setLayout(vlayout);

    stack_ = new QStackedWidget;
    stack_->insertWidget(INDEX_LOADING_VIEW, loading_view_);
    stack_->insertWidget(INDEX_TABLE_VIEW, table_view_);
    stack_->insertWidget(INDEX_LOADING_FAILED_VIEW, loading_failed_view_);

    vlayout->addWidget(toolbar_);
    vlayout->addWidget(stack_);
    vlayout->addWidget(status_bar_);

    // this <--> table_view_
    connect(table_view_, SIGNAL(direntClicked(const SeafDirent&)),
            this, SLOT(onDirentClicked(const SeafDirent&)));
    connect(table_view_, SIGNAL(direntRename(const SeafDirent&)),
            this, SLOT(onGetDirentRename(const SeafDirent&)));
    connect(table_view_, SIGNAL(direntRemove(const SeafDirent&)),
            this, SLOT(onGetDirentRemove(const SeafDirent&)));
    connect(table_view_, SIGNAL(direntShare(const SeafDirent&)),
            this, SLOT(onGetDirentShare(const SeafDirent&)));
    connect(table_view_, SIGNAL(direntUpdate(const SeafDirent&)),
            this, SLOT(onGetDirentUpdate(const SeafDirent&)));

    //dirents <--> data_mgr_
    connect(data_mgr_, SIGNAL(getDirentsSuccess(const QList<SeafDirent>&)),
            this, SLOT(onGetDirentsSuccess(const QList<SeafDirent>&)));
    connect(data_mgr_, SIGNAL(getDirentsFailed(const ApiError&)),
            this, SLOT(onGetDirentsFailed(const ApiError&)));

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

    //share <--> data_mgr_
    connect(data_mgr_, SIGNAL(shareDirentSuccess(const QString&)),
            this, SLOT(onDirentShareSuccess(const QString&)));
    connect(data_mgr_, SIGNAL(shareDirentFailed(const ApiError&)),
            this, SLOT(onDirentShareFailed(const ApiError&)));

    QTimer::singleShot(0, this, SLOT(fetchDirents()));
}

FileBrowserDialog::~FileBrowserDialog()
{
    delete data_mgr_;
}

void FileBrowserDialog::createToolBar()
{
    toolbar_ = new QToolBar;

    const int w = ::getDPIScaledSize(kToolBarIconSize);
    toolbar_->setIconSize(QSize(w, w));

    toolbar_->setObjectName("topBar");
    toolbar_->setIconSize(QSize(w, w));

    backward_action_ = new QAction(tr("Back"), this);
    backward_action_->setIcon(getIconSet(":/images/filebrowser/backward.png", kToolBarIconSize, kToolBarIconSize));
    backward_action_->setEnabled(false);
    toolbar_->addAction(backward_action_);
    connect(backward_action_, SIGNAL(triggered()), this, SLOT(goBackward()));

    forward_action_ = new QAction(tr("Forward"), this);
    forward_action_->setIcon(getIconSet(":/images/filebrowser/forward.png", kToolBarIconSize, kToolBarIconSize));
    forward_action_->setEnabled(false);
    connect(forward_action_, SIGNAL(triggered()), this, SLOT(goForward()));
    toolbar_->addAction(forward_action_);

    gohome_action_ = new QAction(tr("Home"), this);
    gohome_action_->setIcon(getIconSet(":images/filebrowser/home.png", kToolBarIconSize, kToolBarIconSize));
    connect(gohome_action_, SIGNAL(triggered()), this, SLOT(goHome()));
    toolbar_->addAction(gohome_action_);

    path_navigator_ = new QButtonGroup(this);
    connect(path_navigator_, SIGNAL(buttonClicked(int)),
            this, SLOT(onNavigatorClick(int)));

    // root is special
    QPushButton *path_navigator_root_ = new QPushButton(repo_.name);
    path_navigator_root_->setFlat(true);
    path_navigator_root_->setCursor(Qt::PointingHandCursor);
    connect(path_navigator_root_, SIGNAL(clicked()),
            this, SLOT(goHome()));
    toolbar_->addWidget(path_navigator_root_);
}

void FileBrowserDialog::createStatusBar()
{
    status_bar_ = new QToolBar;

    const int w = ::getDPIScaledSize(kStatusBarIconSize);
    status_bar_->setObjectName("statusBar");
    status_bar_->setIconSize(QSize(w, w));

    QWidget *spacer1 = new QWidget;
    spacer1->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
    status_bar_->addWidget(spacer1);

    upload_action_ = new QAction(tr("Upload"), this);
    upload_action_->setIcon(
        getIconSet(":/images/filebrowser/upload.png", kStatusBarIconSize, kStatusBarIconSize));
    connect(upload_action_, SIGNAL(triggered()), this, SLOT(chooseFileToUpload()));
    status_bar_->addAction(upload_action_);
    if (repo_.readonly) {
        upload_action_->setEnabled(false);
        upload_action_->setToolTip(tr("You don't have permission to upload files to this library"));
    }

    details_label_ = new QLabel;
    details_label_->setAlignment(Qt::AlignCenter | Qt::AlignVCenter);
    details_label_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    details_label_->setFixedHeight(w);
    status_bar_->addWidget(details_label_);

    refresh_action_ = new QAction(this);
    refresh_action_->setIcon(
        getIconSet(":/images/filebrowser/refresh.png", kStatusBarIconSize, kStatusBarIconSize));
    connect(refresh_action_, SIGNAL(triggered()), this, SLOT(forceRefresh()));
    status_bar_->addAction(refresh_action_);

    open_cache_dir_action_ = new QAction(this);
    open_cache_dir_action_->setIcon(
        getIconSet(":/images/filebrowser/open-folder.png", kStatusBarIconSize, kStatusBarIconSize));
    connect(open_cache_dir_action_, SIGNAL(triggered()), this, SLOT(openCacheFolder()));
    status_bar_->addAction(open_cache_dir_action_);

    QWidget *spacer2 = new QWidget;
    spacer2->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
    status_bar_->addWidget(spacer2);
}

void FileBrowserDialog::createFileTable()
{
    loading_view_ = new LoadingView;
    table_view_ = new FileTableView(repo_, this);
    table_model_ = new FileTableModel();
    table_view_->setModel(table_model_);

    connect(table_view_, SIGNAL(dropFile(const QString&)),
            this, SLOT(uploadOrUpdateFile(const QString&)));
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
            data_mgr_->setRepoPasswordSet(repo_.id);
        }
    }

    if (!force_refresh) {
        QList<SeafDirent> dirents;
        if (data_mgr_->getDirents(repo_.id, current_path_, &dirents)) {
            updateTable(dirents);
            return;
        }
    }

    showLoading();
    data_mgr_->getDirentsFromServer(repo_.id, current_path_);
}

void FileBrowserDialog::onGetDirentsSuccess(const QList<SeafDirent>& dirents)
{
    updateTable(dirents);
}

void FileBrowserDialog::onGetDirentsFailed(const ApiError& error)
{
    stack_->setCurrentIndex(INDEX_LOADING_FAILED_VIEW);
}

void FileBrowserDialog::createLoadingFailedView()
{
    loading_failed_view_ = new QWidget(this);

    QVBoxLayout *layout = new QVBoxLayout;
    loading_failed_view_->setLayout(layout);

    QLabel *label = new QLabel;
    label->setObjectName(kLoadingFaieldLabelName);
    QString link = QString("<a style=\"color:#777\" href=\"#\">%1</a>").arg(tr("retry"));
    QString label_text = tr("Failed to get files information<br/>"
                            "Please %1").arg(link);
    label->setText(label_text);
    label->setAlignment(Qt::AlignCenter);

    connect(label, SIGNAL(linkActivated(const QString&)),
            this, SLOT(fetchDirents()));

    layout->addWidget(label);
}

void FileBrowserDialog::onDirentClicked(const SeafDirent& dirent)
{
    if (dirent.isDir()) {
        onDirClicked(dirent);
    } else {
        onFileClicked(dirent);
    }
}

void FileBrowserDialog::showLoading()
{
    forward_action_->setEnabled(false);
    backward_action_->setEnabled(false);
    upload_action_->setEnabled(false);
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
    foreach(QAbstractButton *button, buttons)
    {
        path_navigator_->removeButton(button);
        delete button;
    }
    foreach(QLabel *label, path_navigator_separators_)
    {
        delete label;
    }
    path_navigator_separators_.clear();


    // add new buttons for navigator except the root
    for(int i = 1; i < current_lpath_.size(); i++) {
        QLabel *separator = new QLabel("/");
        path_navigator_separators_.push_back(separator);
        toolbar_->addWidget(separator);
        QPushButton* button = new QPushButton(current_lpath_[i]);
        button->setFlat(true);
        button->setCursor(Qt::PointingHandCursor);
        path_navigator_->addButton(button, i);
        toolbar_->addWidget(button);
    }
}

void FileBrowserDialog::onFileClicked(const SeafDirent& file)
{
    QString fpath = ::pathJoin(current_path_, file.name);

    QString cached_file = data_mgr_->getLocalCachedFile(repo_.id, fpath, file.id);
    if (!cached_file.isEmpty()) {
        openFile(cached_file);
        return;
    } else {
        downloadFile(fpath);
    }
}

void FileBrowserDialog::downloadFile(const QString& path)
{
    FileDownloadTask *task = data_mgr_->createDownloadTask(repo_.id, path);
    FileBrowserProgressDialog dialog(task, this);
    connect(task, SIGNAL(finished(bool)),
            this, SLOT(onDownloadFinished(bool)));
    task->start();
    dialog.exec();
}

void FileBrowserDialog::uploadFile(const QString& path, const QString& name,
                                   const bool overwrite)
{
    FileUploadTask *task =
      data_mgr_->createUploadTask(repo_.id, current_path_, path, name, overwrite);
    FileBrowserProgressDialog dialog(task, this);
    connect(task, SIGNAL(finished(bool)), this, SLOT(onUploadFinished(bool)));
    task->start();
    dialog.exec();
}

void FileBrowserDialog::uploadOrUpdateFile(const QString& path)
{
    const QString name = QFileInfo(path).fileName();

    bool found_conflict = false;
    // find if there exist a file with the same name
    Q_FOREACH(const SeafDirent &dirent, table_model_->dirents())
    {
        if (dirent.name == name) {
            found_conflict = true;
            break;
        }
    }

    // prompt a dialog to confirm to overwrite the current file
    if (found_conflict) {
        int ret = QMessageBox::question(
            this, getBrand(),
            tr("File %1 already exists.<br/>"
            "Do you like to overwrite it?<br/>"
            "<small>(Choose No to upload using an alternative name).</small>").
            arg(name),
            QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
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

void FileBrowserDialog::onDownloadFinished(bool success)
{
    FileNetworkTask *task = qobject_cast<FileNetworkTask *>(sender());
    if (task == NULL)
        return;
    if (success) {
        openFile(task->localFilePath());
    } else {
        if (repo_.encrypted &&
            setPasswordAndRetry(task)) {
            return;
        }

        QString msg = tr("Failed to download file: %1").arg(task->errorString());
        seafApplet->warningBox(msg, this);
    }
}

void FileBrowserDialog::onUploadFinished(bool success)
{
    FileUploadTask *task = qobject_cast<FileUploadTask *>(sender());
    if (task == NULL)
        return;
    if (success) {
        const QFileInfo file = task->localFilePath();
        const SeafDirent dirent = {
          SeafDirent::FILE,
          task->oid(),
          task->name(),
          file.size(),
          file.lastModified().toTime_t()
        };
        if (task->useUpload())
            table_model_->appendItem(dirent);
        else
            table_model_->replaceItem(task->name(), dirent);
    } else {
        if (repo_.encrypted &&
            setPasswordAndRetry(task)) {
            return;
        }

        QString msg = tr("Failed to upload file: %1").arg(task->errorString());
        seafApplet->warningBox(msg, this);
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
    QString path = forward_history_.pop();
    backward_history_.push(current_path_);
    enterPath(path);
}

void FileBrowserDialog::goBackward()
{
    QString path = backward_history_.pop();
    forward_history_.push(current_path_);
    enterPath(path);
}

void FileBrowserDialog::goHome()
{
    if (current_path_ == "/") {
        return;
    }
    QString path = "/";
    backward_history_.push(current_path_);
    enterPath(path);
}

void FileBrowserDialog::updateTable(const QList<SeafDirent>& dirents)
{
    table_model_->setDirents(dirents);
    stack_->setCurrentIndex(INDEX_TABLE_VIEW);
    if (!forward_history_.empty()) {
        forward_action_->setEnabled(true);
    } else {
        forward_action_->setEnabled(false);
    }
    if (!backward_history_.empty()) {
        backward_action_->setEnabled(true);
    } else {
        backward_action_->setEnabled(false);
    }
    if (!repo_.readonly) {
        upload_action_->setEnabled(true);
    }
    gohome_action_->setEnabled(true);
}

void FileBrowserDialog::chooseFileToUpload()
{
    QString path = QFileDialog::getOpenFileName(this, tr("Select a file to upload"));
    if (!path.isEmpty()) {
        uploadOrUpdateFile(path);
    }
}

void FileBrowserDialog::openCacheFolder()
{
    QString folder = data_mgr_->getRepoCacheFolder(repo_.id);
    ::createDirIfNotExists(folder);
    QDesktopServices::openUrl(QUrl::fromLocalFile(folder));
}

void FileBrowserDialog::onNavigatorClick(int id)
{
    // root is safe, since it is handled by goHome not this function
    // i.e. id is never equal to 0

    // calculate the path
    QString path = "/";
    for(int i = 1; i <= id; i++)
      path += current_lpath_[i] + "/";

    enterPath(path);
}

void FileBrowserDialog::onGetDirentRename(const SeafDirent& dirent,
                                          QString new_name)
{
    if (new_name.isEmpty()) {
        new_name = QInputDialog::getText(this, tr("Rename"),
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
    if (seafApplet->yesOrNoBox(dirent.isFile() ?
            tr("Do you really want to delete file \"%1\"").arg(dirent.name) :
            tr("Do you really want to delete folder \"%1\"").arg(dirent.name), this, false))
        data_mgr_->removeDirent(repo_.id, pathJoin(current_path_, dirent.name),
                                dirent.isFile());
}

void FileBrowserDialog::onGetDirentShare(const SeafDirent& dirent)
{
    data_mgr_->shareDirent(repo_.id,
                           ::pathJoin(current_path_, dirent.name),
                           dirent.isFile());
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
        tr("Select a file to update %1").arg(dirent.name));
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

void FileBrowserDialog::onDirentShareSuccess(const QString &link)
{
    SharedLinkDialog(link, this).exec();
}

void FileBrowserDialog::onDirentShareFailed(const ApiError&error)
{
    seafApplet->warningBox(tr("Share failed"), this);
}

