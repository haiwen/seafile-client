#include <QtGui>
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>

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

#include "file-browser-dialog.h"

namespace {

enum {
    INDEX_LOADING_VIEW = 0,
    INDEX_TABLE_VIEW,
    INDEX_LOADING_FAILED_VIEW,
};

const char *kLoadingFaieldLabelName = "loadingFailedText";
const int kToolBarIconSize = 20;
const int kStatusBarIconSize = 24;

} // namespace

FileBrowserDialog::FileBrowserDialog(const ServerRepo& repo, QWidget *parent)
    : QDialog(parent),
      repo_(repo)
{
    current_path_ = "/";

    const Account& account = seafApplet->accountManager()->currentAccount();
    data_mgr_ = new DataManager(account);

    setWindowTitle(tr("File Browser"));
    setWindowIcon(QIcon(":/images/seafile.png"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

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

    connect(table_view_, SIGNAL(direntClicked(const SeafDirent&)),
            this, SLOT(onDirentClicked(const SeafDirent&)));

    connect(data_mgr_, SIGNAL(getDirentsSuccess(const QList<SeafDirent>&)),
            this, SLOT(onGetDirentsSuccess(const QList<SeafDirent>&)));
    connect(data_mgr_, SIGNAL(getDirentsFailed(const ApiError&)),
            this, SLOT(onGetDirentsFailed(const ApiError&)));

    fetchDirents();
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
    backward_action_->setIcon(getIconSet(":images/filebrowser/backward.png", kToolBarIconSize, kToolBarIconSize));
    backward_action_->setEnabled(false);
    toolbar_->addAction(backward_action_);
    connect(backward_action_, SIGNAL(triggered()), this, SLOT(goBackward()));

    forward_action_ = new QAction(tr("Forward"), this);
    forward_action_->setIcon(getIconSet(":images/filebrowser/forward.png", kToolBarIconSize, kToolBarIconSize));
    forward_action_->setEnabled(false);
    connect(forward_action_, SIGNAL(triggered()), this, SLOT(goForward()));
    toolbar_->addAction(forward_action_);

    gohome_action_ = new QAction(tr("Home"), this);
    gohome_action_->setIcon(getIconSet(":images/filebrowser/home.png", kToolBarIconSize, kToolBarIconSize));
    connect(gohome_action_, SIGNAL(triggered()), this, SLOT(goHome()));
    toolbar_->addAction(gohome_action_);

    path_line_edit_ = new QLineEdit;
    path_line_edit_->setReadOnly(true);
    path_line_edit_->setText("/");
    path_line_edit_->setAlignment(Qt::AlignHCenter | Qt::AlignLeft);
    path_line_edit_->setMaximumHeight(kToolBarIconSize);
    path_line_edit_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    toolbar_->addWidget(path_line_edit_);
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

    upload_action_ = new QAction(this);
    upload_action_->setIcon(
        getIconSet(":images/filebrowser/upload.png", kStatusBarIconSize, kStatusBarIconSize));
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
        getIconSet(":images/filebrowser/refresh.png", kStatusBarIconSize, kStatusBarIconSize));
    connect(refresh_action_, SIGNAL(triggered()), this, SLOT(forceRefresh()));
    status_bar_->addAction(refresh_action_);

    open_cache_dir_action_ = new QAction(this);
    open_cache_dir_action_->setIcon(
        getIconSet(":/images/filebrowser/open-folder.png", kStatusBarIconSize, kStatusBarIconSize));
    // connect(open_cache_dir_action_, SIGNAL(triggered()), this, SLOT(openCacheFolder()));
    status_bar_->addAction(open_cache_dir_action_);

    QWidget *spacer2 = new QWidget;
    spacer2->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
    status_bar_->addWidget(spacer2);
}

void FileBrowserDialog::createFileTable()
{
    loading_view_ = new LoadingView;
    table_view_ = new FileTableView(repo_);
    table_model_ = new FileTableModel();
    table_view_->setModel(table_model_);
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
    fetchDirents();
    path_line_edit_->setText(current_path_);
}

void FileBrowserDialog::onFileClicked(const SeafDirent& file)
{
    QString fpath = ::pathJoin(current_path_, file.name);

    QString cached_file = data_mgr_->getLocalCachedFile(repo_.id, fpath, file.id);
    if (!cached_file.isEmpty()) {
        openFile(cached_file);
        return;
    } else {
        FileDownloadTask *task = data_mgr_->createDownloadTask(repo_.id, fpath);
        connect(task, SIGNAL(finished(bool)),
                this, SLOT(onDownloadFinished(bool)));
        FileBrowserProgressDialog dialog(task, this);
        task->start();
        dialog.exec();
    }
}

void FileBrowserDialog::onDownloadFinished(bool success)
{
    FileDownloadTask *task = (FileDownloadTask *)sender();
    if (success) {
        openFile(task->localFilePath());
    } else {
        const FileNetworkTask::TaskError& error = task->error();
        if (error != FileNetworkTask::NoError) {
            QString msg = tr("Failed to download file: %1").arg(task->errorString());
            seafApplet->warningBox(msg, this);
        }
    }
}

void FileBrowserDialog::openFile(const QString& path)
{
    openInNativeExtension(path) || showInGraphicalShell(path);
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
    QString path = QFileDialog::getOpenFileName(this, tr("Select file to upload"));
    if (!path.isEmpty()) {
        FileUploadTask *task = data_mgr_->createUploadTask(repo_.id, current_path_, path);
        FileBrowserProgressDialog dialog(task, this);
        connect(task, SIGNAL(finished(bool)),
                this, SLOT(onUploadFinished(bool)));
        task->start();
        dialog.exec();
    }
}

void FileBrowserDialog::onUploadFinished(bool success)
{
    if (success) {
        forceRefresh();
    } else {
        FileUploadTask *task = (FileUploadTask *)sender();
        const FileNetworkTask::TaskError& error = task->error();
        if (error != FileNetworkTask::NoError) {
            QString msg = tr("Failed to upload file: %1").arg(task->errorString());
            seafApplet->warningBox(msg, this);
        }
    }
}
