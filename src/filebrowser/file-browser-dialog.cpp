#include "file-browser-dialog.h"

#include <QtGui>

#include "ui/loading-view.h"
#include "utils/paint-utils.h"

#include "seaf-dirent.h"
#include "file-table-model.h"
#include "file-table-view.h"

#include "file-browser-progress-dialog.h"

namespace {

enum {
    INDEX_LOADING_VIEW = 0,
    INDEX_TABLE_VIEW,
    INDEX_LOADING_FAILED_VIEW
};

const char kLoadingFaieldLabelName[] = "loadingFailedText";
const int kToolBarIconSize = 20;
const int kStatusBarIconSize = 24;

} // namespace

FileBrowserDialog::FileBrowserDialog(const ServerRepo& repo, QWidget *parent)
    : QDialog(parent),
    repo_id_and_path_(tr("Library \"%1\": %2").arg(repo.name)),
    table_model_(new FileTableModel(repo, this))
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle(tr("Cloud File Browser - %1").arg(table_model_->account().serverUrl.toString()));
    setWindowIcon(QIcon(":/images/seafile.png"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    layout_ = new QVBoxLayout;
    setLayout(layout_);
    layout_->setContentsMargins(0, 6, 0, 0);
    layout_->setSpacing(0);

    createToolBar();
    createLoadingFailedView();
    createFileTable();
    createStatusBar();
    createStackWidget();

    layout_->addWidget(toolbar_);
    layout_->addWidget(stack_);
    layout_->addWidget(status_bar_);

    connect(table_model_, SIGNAL(loading()), this, SLOT(onLoading()));
    connect(table_model_, SIGNAL(loadingFinished()),
            this, SLOT(onLoadingFinished()));
    connect(table_model_, SIGNAL(loadingFailed()),
            this, SLOT(onLoadingFailed()));

    file_progress_dialog_ = new FileBrowserProgressDialog(this);
    file_progress_dialog_->setFileNetworkManager(table_model_->fileNetworkManager());

    resize(size());
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
    toolbar_->setObjectName("topBar");
    toolbar_->setIconSize(QSize(w, w));

    backward_action_ = new QAction(tr("Back"), layout_);
    backward_action_->setIcon(getIconSet(":images/filebrowser/backward.png", kToolBarIconSize, kToolBarIconSize));
    backward_action_->setEnabled(false);
    toolbar_->addAction(backward_action_);
    connect(backward_action_, SIGNAL(triggered()), table_model_, SLOT(onBackward()));
    connect(table_model_, SIGNAL(backwardEnabled(bool)), this, SLOT(onBackwardEnabled(bool)));

    forward_action_ = new QAction(tr("Forward"), layout_);
    forward_action_->setIcon(getIconSet(":images/filebrowser/forward.png", kToolBarIconSize, kToolBarIconSize));
    forward_action_->setEnabled(false);
    connect(forward_action_, SIGNAL(triggered()), table_model_, SLOT(onForward()));
    connect(table_model_, SIGNAL(forwardEnabled(bool)), this, SLOT(onForwardEnabled(bool)));
    toolbar_->addAction(forward_action_);

    navigate_home_action_ = new QAction(tr("Home"), layout_);
    navigate_home_action_->setIcon(getIconSet(":images/filebrowser/home.png", kToolBarIconSize, kToolBarIconSize));
    connect(navigate_home_action_, SIGNAL(triggered()), table_model_, SLOT(onNavigateHome()));
    toolbar_->addAction(navigate_home_action_);

    path_line_edit_ = new QLineEdit;
    path_line_edit_->setReadOnly(true);
    path_line_edit_->setText(repo_id_and_path_.arg("/"));
    path_line_edit_->setAlignment(Qt::AlignHCenter | Qt::AlignLeft);
    path_line_edit_->setMaximumHeight(kToolBarIconSize);
    toolbar_->addWidget(path_line_edit_);
}

void FileBrowserDialog::createFileTable()
{
    loading_view_ = new LoadingView;
    table_view_ = new FileTableView;
    table_view_->setModel(table_model_); // connect is called inside setModel
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
    upload_action_->setIcon(getIconSet(":images/filebrowser/upload.png", kStatusBarIconSize, kStatusBarIconSize));
    connect(upload_action_, SIGNAL(triggered()), table_model_, SLOT(onFileUpload()));
    status_bar_->addAction(upload_action_);

    details_label_ = new QLabel;
    details_label_->setAlignment(Qt::AlignCenter | Qt::AlignVCenter);
    details_label_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    details_label_->setFixedHeight(w);
    status_bar_->addWidget(details_label_);

    refresh_action_ = new QAction(this);
    refresh_action_->setIcon(getIconSet(":images/filebrowser/refresh.png", kStatusBarIconSize, kStatusBarIconSize));
    connect(refresh_action_, SIGNAL(triggered()), table_model_, SLOT(onRefreshForcely()));
    status_bar_->addAction(refresh_action_);

    open_cache_dir_action_ = new QAction(this);
    open_cache_dir_action_->setIcon(getIconSet(":/images/filebrowser/open-folder.png", kStatusBarIconSize, kStatusBarIconSize));
    connect(open_cache_dir_action_, SIGNAL(triggered()), table_model_, SLOT(onOpenCacheDir()));
    status_bar_->addAction(open_cache_dir_action_);

    QWidget *spacer2 = new QWidget;
    spacer2->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
    status_bar_->addWidget(spacer2);
}

void FileBrowserDialog::createStackWidget()
{
    stack_ = new QStackedWidget;
    stack_->insertWidget(INDEX_LOADING_VIEW, loading_view_);
    stack_->insertWidget(INDEX_TABLE_VIEW, table_view_);
    stack_->insertWidget(INDEX_LOADING_FAILED_VIEW, loading_failed_view_);
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
    toolbar_->setEnabled(false);
    upload_action_->setEnabled(false);
    refresh_action_->setEnabled(false);

    details_label_->setText(tr("Loading..."));
    path_line_edit_->setText(repo_id_and_path_.arg(table_model_->currentPath()));
    stack_->setCurrentIndex(INDEX_LOADING_VIEW);
}

void FileBrowserDialog::onLoadingFinished()
{
    toolbar_->setEnabled(true);
    upload_action_->setEnabled(true);
    refresh_action_->setEnabled(true);

    details_label_->setText(tr("Loading..."));
    details_label_->setText(
        tr("%1 files and directories found").arg(table_model_->dirents().size()));
    stack_->setCurrentIndex(INDEX_TABLE_VIEW);
}

void FileBrowserDialog::onLoadingFailed()
{
    toolbar_->setEnabled(true);
    refresh_action_->setEnabled(true);

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

void FileBrowserDialog::resizeEvent(QResizeEvent *event)
{
    QDialog::resizeEvent(event);
    table_model_->onResizeEvent(event->size());
}
