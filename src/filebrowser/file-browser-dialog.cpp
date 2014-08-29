#include <QtGui>

#include "seafile-applet.h"
#include "account-mgr.h"
#include "utils/widget-utils.h"
#include "utils/paint-utils.h"
#include "api/api-error.h"
#include "file-table.h"
#include "seaf-dirent.h"
#include "data-mgr.h"
#include "data-mgr.h"

#include "file-browser-dialog.h"

namespace {

enum {
    INDEX_LOADING_VIEW = 0,
    INDEX_TABLE_VIEW,
    INDEX_LOADING_FAILED_VIEW,
};

const char *kLoadingFaieldLabelName = "loadingFailedText";

} // namespace

FileBrowserDialog::FileBrowserDialog(const ServerRepo& repo, QWidget *parent)
    : QDialog(parent),
      repo_(repo)
{
    path_ = "/";

    const Account& account = seafApplet->accountManager()->currentAccount();
    data_mgr_ = new DataManager(account);

    setWindowTitle(tr("File Browser"));
    setWindowIcon(QIcon(":/images/seafile.png"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    createToolBar();
    createLoadingFailedView();
    createFileTable();

    QVBoxLayout *vlayout = new QVBoxLayout;
    setLayout(vlayout);

    stack_ = new QStackedWidget;
    stack_->insertWidget(INDEX_LOADING_VIEW, loading_view_);
    stack_->insertWidget(INDEX_TABLE_VIEW, table_view_);
    stack_->insertWidget(INDEX_LOADING_FAILED_VIEW, loading_failed_view_);

    vlayout->addWidget(toolbar_);
    vlayout->addWidget(stack_);

    connect(table_view_, SIGNAL(direntClicked(const SeafDirent&)),
            this, SLOT(onDirentClicked(const SeafDirent&)));

    connect(data_mgr_, SIGNAL(getDirentsSuccess(const std::vector<SeafDirent>&)),
            this, SLOT(onGetDirentsSuccess(const std::vector<SeafDirent>&)));
    connect(data_mgr_, SIGNAL(failed(const ApiError&)),
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

    int w = ::getDPIScaledSize(24);
    toolbar_->setIconSize(QSize(w, w));

    QWidget *spacer = new QWidget;
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    toolbar_->addWidget(spacer);

    refresh_action_ = new QAction(tr("Refresh"), this);
    refresh_action_->setIcon(QIcon(":/images/refresh.png"));
    connect(refresh_action_, SIGNAL(triggered()), this, SLOT(fetchDirents()));
    toolbar_->addAction(refresh_action_);
}

void FileBrowserDialog::createFileTable()
{
    loading_view_ = ::newLoadingView();
    table_view_ = new FileTableView(repo_);
    table_model_ = new FileTableModel();
    table_view_->setModel(table_model_);
}

void FileBrowserDialog::fetchDirents()
{
    stack_->setCurrentIndex(INDEX_LOADING_VIEW);
    data_mgr_->getDirents(repo_.id, path_);
}

void FileBrowserDialog::onGetDirentsSuccess(const std::vector<SeafDirent>& dirents)
{
    stack_->setCurrentIndex(INDEX_TABLE_VIEW);
    table_model_->setDirents(dirents);
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

void FileBrowserDialog::onDirClicked(const SeafDirent& dir)
{
    // TODO: handle go up a level
    if (!path_.endsWith("/")) {
        path_ += "/";
    }

    path_ += dir.name;

    printf ("nav to %s\n", path_.toUtf8().data());;

    fetchDirents();
}

void FileBrowserDialog::onFileClicked(const SeafDirent& file)
{
}
