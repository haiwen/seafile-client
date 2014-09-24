#include "file-browser-dialog.h"

#include <QtGui>
#include <QApplication>

#include "ui/loading-view.h"
#include "utils/paint-utils.h"

#include "file-table.h"
#include "file-table-view.h"
#include "seaf-dirent.h"

#include "file-browser-progress-dialog.h"

namespace {

enum {
    INDEX_LOADING_VIEW = 0,
    INDEX_TABLE_VIEW,
    INDEX_LOADING_FAILED_VIEW,
};

const char kLoadingFaieldLabelName[] = "loadingFailedText";
const int kToolBarHeight = 36;
const int kToolBarIconSize = 32;
const int kStatusBarIconSize = 16;

} // namespace

FileBrowserDialog::FileBrowserDialog(const ServerRepo& repo, QWidget *parent)
    : QDialog(parent),
    repo_id_and_path_(tr("Library \"%1\": %2").arg(repo.name)),
    table_model_(new FileTableModel(repo, this))
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle(tr("File Browser - %1").arg(table_model_->account().serverUrl.toString()));
    setWindowIcon(QIcon(":/images/seafile.png"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    layout_ = new QVBoxLayout;
    setLayout(layout_);
    layout_->setContentsMargins(0, 6, 0, 0);

    createToolBar();
    createLoadingFailedView();
    createFileTable();
    createStatusBar();
    createStackWidget();

    layout_->addWidget(toolbar_);
    layout_->addWidget(stack_);
    layout_->addWidget(status_bar_, 0, Qt::AlignBottom);

    connect(table_model_, SIGNAL(loading()), this, SLOT(onLoading()));
    connect(table_model_, SIGNAL(loadingFinished()),
            this, SLOT(onLoadingFinished()));
    connect(table_model_, SIGNAL(loadingFailed()),
            this, SLOT(onLoadingFailed()));

    file_progress_dialog_ = new FileBrowserProgressDialog(this);
    file_progress_dialog_->setWindowModality(Qt::WindowModal);
    connect(table_model_, SIGNAL(taskCreated(const FileNetworkTask*)),
            this, SLOT(onTaskCreated(const FileNetworkTask*)));

    table_model_->onRefresh();
}

FileBrowserDialog::~FileBrowserDialog()
{
    // empty
}

void FileBrowserDialog::createToolBar()
{
    toolbar_ = new QToolBar;

    const int w = ::getDPIScaledSize(kToolBarIconSize);
    toolbar_->setIconSize(QSize(w, w));
    toolbar_->setMaximumHeight(kToolBarHeight);
    toolbar_->setMinimumHeight(kToolBarHeight);

    backward_action_ = new QAction(tr("Back"), layout_);
    backward_action_->setIcon(QApplication::style()->standardIcon(QStyle::SP_ArrowBack));
    backward_action_->setEnabled(false);
    toolbar_->addAction(backward_action_);
    connect(backward_action_, SIGNAL(triggered()), table_model_, SLOT(onBackward()));
    connect(table_model_, SIGNAL(backwardEnabled(bool)), this, SLOT(onBackwardEnabled(bool)));

    forward_action_ = new QAction(tr("Forward"), layout_);
    forward_action_->setIcon(QApplication::style()->standardIcon(QStyle::SP_ArrowForward));
    forward_action_->setEnabled(false);
    connect(forward_action_, SIGNAL(triggered()), table_model_, SLOT(onForward()));
    connect(table_model_, SIGNAL(forwardEnabled(bool)), this, SLOT(onForwardEnabled(bool)));
    toolbar_->addAction(forward_action_);

    navigate_home_action_ = new QAction(tr("Home"), layout_);
    navigate_home_action_->setIcon(QApplication::style()->standardIcon(QStyle::SP_DirHomeIcon));
    connect(navigate_home_action_, SIGNAL(triggered()), table_model_, SLOT(onNavigateHome()));
    toolbar_->addAction(navigate_home_action_);

    path_line_edit_ = new QLineEdit;
    path_line_edit_->setReadOnly(true);
    path_line_edit_->setText(repo_id_and_path_.arg("/"));
    path_line_edit_->setAlignment(Qt::AlignHCenter | Qt::AlignLeft);
    path_line_edit_->setMaximumHeight(kToolBarIconSize);
    path_line_edit_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    path_line_edit_->setStyleSheet("border-radius: 5px; margin-right: 12px");
    toolbar_->addWidget(path_line_edit_);
}

void FileBrowserDialog::createFileTable()
{
    loading_view_ = new LoadingView;
    table_view_ = new FileTableView;
    table_view_->setModel(table_model_); // connect is called inside setModel
    table_view_->setContentsMargins(0,0,0,0);
}

void FileBrowserDialog::createStatusBar()
{
    status_bar_ = new QToolBar;

    const int w = ::getDPIScaledSize(kStatusBarIconSize);
    status_bar_->setIconSize(QSize(w, w));
    status_bar_->setContentsMargins(10, -10, 10, 14);
    status_bar_->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

    settings_action_ = new QAction(this);
    settings_action_->setIcon(QIcon(":/images/account-settings.png"));
    settings_action_->setEnabled(false);
    status_bar_->addAction(settings_action_);

    QWidget *spacer1 = new QWidget;
    spacer1->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
    status_bar_->addWidget(spacer1);

    upload_action_ = new QAction(this);
    upload_action_->setIcon(QIcon(":/images/plus.png"));
    connect(upload_action_, SIGNAL(triggered()), table_model_, SLOT(onFileUpload()));
    status_bar_->addAction(upload_action_);

    download_action_ = new QAction(this);
    download_action_->setIcon(QIcon(":/images/download.png"));
    connect(table_model_, SIGNAL(downloadEnabled(bool)), this, SLOT(onDownloadEnabled(bool)));
    connect(download_action_, SIGNAL(triggered()), table_model_, SLOT(onFileDownload()));
    status_bar_->addAction(download_action_);

    details_label_ = new QLabel;
    details_label_->setAlignment(Qt::AlignCenter);
    details_label_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    status_bar_->addWidget(details_label_);

    refresh_action_ = new QAction(this);
    refresh_action_->setIcon(QIcon(":/images/refresh.png"));
    connect(refresh_action_, SIGNAL(triggered()), table_model_, SLOT(onRefreshForcely()));
    status_bar_->addAction(refresh_action_);

    QWidget *spacer2 = new QWidget;
    spacer2->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    status_bar_->addWidget(spacer2);

    open_cache_dir_action_ = new QAction(this);
    open_cache_dir_action_->setIcon(QIcon(":/images/folder-open.png"));
    connect(open_cache_dir_action_, SIGNAL(triggered()), table_model_, SLOT(onOpenCacheDir()));
    status_bar_->addAction(open_cache_dir_action_);
}

void FileBrowserDialog::createStackWidget()
{
    stack_ = new QStackedWidget;
    stack_->insertWidget(INDEX_LOADING_VIEW, loading_view_);
    stack_->insertWidget(INDEX_TABLE_VIEW, table_view_);
    stack_->insertWidget(INDEX_LOADING_FAILED_VIEW, loading_failed_view_);
    stack_->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
    stack_->setContentsMargins(0, 0, 0, -14);
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
            table_model_, SLOT(onRefresh()));

    layout->addWidget(label);
}

void FileBrowserDialog::onLoading()
{
    details_label_->setText(tr("Loading..."));
    path_line_edit_->setText(repo_id_and_path_.arg(table_model_->currentPath()));
    stack_->setCurrentIndex(INDEX_LOADING_VIEW);
}

void FileBrowserDialog::onLoadingFinished()
{
    details_label_->setText(
        tr("%1 files and directories found").arg(table_model_->dirents().size()));
    stack_->setCurrentIndex(INDEX_TABLE_VIEW);
}

void FileBrowserDialog::onLoadingFailed()
{
    details_label_->setText(tr("Failed to load at path: %1").arg(table_model_->currentPath()));
    stack_->setCurrentIndex(INDEX_LOADING_FAILED_VIEW);
}

void FileBrowserDialog::onBackwardEnabled(bool enabled)
{
    backward_action_->setEnabled(enabled);
}

void FileBrowserDialog::onForwardEnabled(bool enabled)
{
    forward_action_->setEnabled(enabled);
}

void FileBrowserDialog::onDownloadEnabled(bool enabled)
{
    download_action_->setEnabled(enabled);
}

void FileBrowserDialog::onTaskCreated(const FileNetworkTask *task)
{
    file_progress_dialog_->setTask(task);
    file_progress_dialog_->show();
}
