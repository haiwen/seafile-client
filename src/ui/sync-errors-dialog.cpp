#include <QtGlobal>

#include <QtWidgets>
#include <QTableView>
#include <QResizeEvent>
#include <QTimer>
#include <QDesktopServices>
#include <QCloseEvent>

#include "QtAwesome.h"
#include "utils/utils.h"
#include "seafile-applet.h"
#include "rpc/rpc-client.h"
#include "rpc/sync-error.h"
#include "rpc/local-repo.h"
#include "sync-errors-dialog.h"

namespace {

const int kUpdateErrorsIntervalMSecs = 3000;

const int kDefaultColumnWidth = 120;
const int kDefaultColumnHeight = 40;

const int kRepoNameColumnWidth = 100;
const int kPathColumnWidth = 150;
const int kErrorColumnWidth = 200;
const int kTimestampColumnWidth = 80;
const int kExtraPadding = 80;

const int kDefaultColumnSum = kRepoNameColumnWidth + kPathColumnWidth + kErrorColumnWidth + kTimestampColumnWidth + kExtraPadding;

enum {
    INDEX_EMPTY_VIEW = 0,
    INDEX_TABE_VIEW
};

enum {
    COLUMN_REPO_NAME = 0,
    COLUMN_PATH,
    COLUMN_ERROR_STR,
    COLUMN_TIMESTAMP,
    MAX_COLUMN,
};


} // namespace

// TODO: There are lots of common logic used in FileBrowserDialog and
// SyncErrorsDialog. We should refactor out a base dialog class.
SyncErrorsDialog::SyncErrorsDialog(QWidget *parent)
    : QDialog(parent)
{
    // setupUi(this);

    setWindowTitle(tr("File Sync Errors"));
    setWindowIcon(QIcon(":/images/seafile.png"));

    Qt::WindowFlags flags =
        (windowFlags() & ~Qt::WindowContextHelpButtonHint & ~Qt::Dialog) |
        Qt::Window | Qt::WindowSystemMenuHint | Qt::CustomizeWindowHint |
        Qt::WindowMinimizeButtonHint | Qt::WindowCloseButtonHint |
        Qt::WindowMaximizeButtonHint;

    setWindowFlags(flags);

    createEmptyView();

    table_ = new SyncErrorsTableView;
    model_ = new SyncErrorsTableModel(this);
    table_->setModel(model_);

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
    stack_->insertWidget(INDEX_EMPTY_VIEW, empty_view_);
    stack_->insertWidget(INDEX_TABE_VIEW, table_);
    stack_->setContentsMargins(0, 0, 0, 0);

    vlayout->addWidget(stack_);

    onModelReset();
    connect(model_, SIGNAL(modelReset()), this, SLOT(onModelReset()));
}

void SyncErrorsDialog::closeEvent(QCloseEvent *event)
{
    event->ignore();
    this->hide();
}

void SyncErrorsDialog::updateErrors()
{
    model_->updateErrors();
}

void SyncErrorsDialog::onModelReset()
{
    if (model_->rowCount() == 0) {
        stack_->setCurrentIndex(INDEX_EMPTY_VIEW);
    } else {
        stack_->setCurrentIndex(INDEX_TABE_VIEW);
    }
}


void SyncErrorsDialog::createEmptyView()
{
    empty_view_ = new QWidget(this);

    QVBoxLayout *layout = new QVBoxLayout;
    empty_view_->setLayout(layout);

    QLabel *label = new QLabel;
    label->setText(tr("No sync errors."));
    label->setAlignment(Qt::AlignCenter);

    layout->addWidget(label);
}

SyncErrorsTableView::SyncErrorsTableView(QWidget *parent)
    : QTableView(parent)
{
    verticalHeader()->hide();
    verticalHeader()->setDefaultSectionSize(36);
    horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    horizontalHeader()->setStretchLastSection(true);
    horizontalHeader()->setCascadingSectionResizes(true);
    horizontalHeader()->setHighlightSections(false);
    horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    setGridStyle(Qt::NoPen);
    setShowGrid(false);
    setContentsMargins(0, 0, 0, 0);
    setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setMouseTracking(true);

    createContextMenu();

    connect(this, SIGNAL(doubleClicked(const QModelIndex&)),
            this, SLOT(onItemDoubleClicked(const QModelIndex&)));
}

void SyncErrorsTableView::contextMenuEvent(QContextMenuEvent *event)
{
    QPoint pos = event->pos();
    int row = rowAt(pos.y());
    qDebug("row = %d\n", row);
    if (row == -1) {
        return;
    }

    SyncErrorsTableModel *model = (SyncErrorsTableModel *)this->model();

    SyncError error = model->errorAt(row);

    prepareContextMenu(error);
    pos = viewport()->mapToGlobal(pos);
    context_menu_->exec(pos);
}

void SyncErrorsTableView::prepareContextMenu(const SyncError& error)
{
}

void SyncErrorsTableView::createContextMenu()
{
    context_menu_ = new QMenu(this);
}

void SyncErrorsTableView::resizeEvent(QResizeEvent *event)
{
    QTableView::resizeEvent(event);
    SyncErrorsTableModel *m = (SyncErrorsTableModel *)(model());
    m->onResize(event->size());
}

void SyncErrorsTableView::onItemDoubleClicked(const QModelIndex& index)
{
    SyncErrorsTableModel *model = (SyncErrorsTableModel *)this->model();
    SyncError error = model->errorAt(index.row());

    // printf("error repo id is %s\n", error.repo_id.toUtf8().data());
    if (!error.repo_id.isEmpty()) {
        LocalRepo repo;
        seafApplet->rpcClient()->getLocalRepo(error.repo_id, &repo);
        if (repo.isValid()) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(repo.worktree));
        }
    }
}

SyncErrorsTableModel::SyncErrorsTableModel(QObject *parent)
    : QAbstractTableModel(parent),
      repo_name_column_width_(kRepoNameColumnWidth),
      path_column_width_(kPathColumnWidth),
      error_column_width_(kErrorColumnWidth)
{
    update_timer_ = new QTimer(this);
    connect(update_timer_, SIGNAL(timeout()), this, SLOT(updateErrors()));
    update_timer_->start(kUpdateErrorsIntervalMSecs);

    updateErrors();
}

void SyncErrorsTableModel::updateErrors()
{
    std::vector<SyncError> errors;
    bool success = seafApplet->rpcClient()->getSyncErrors(&errors, 0, 50);
    if (!success) {
        qDebug("failed to get sync errors");
        return;
    }

    // SyncError fake_error;
    // fake_error.repo_id = "xxx";
    // fake_error.repo_name = "NotSoGood";
    // fake_error.path = "/tmp/NotSoGood/BadFile";
    // fake_error.error_id = 5;
    // fake_error.timestamp = 1483056000;
    // fake_error.translateErrorStr();
    // errors.push_back(fake_error);

    if (errors_ == errors) {
        return;
    }

    if (errors_.size() != errors.size()) {
        beginResetModel();
        errors_ = errors;
        endResetModel();
        return;
    }

    for (int i = 0, n = errors.size(); i < n; i++) {
        if (errors_[i] == errors[i]) {
            continue;
        }

        errors_[i] = errors[i];
        QModelIndex start = QModelIndex().child(i, 0);
        QModelIndex stop = QModelIndex().child(i, MAX_COLUMN - 1);
        emit dataChanged(start, stop);
    }
}

int SyncErrorsTableModel::rowCount(const QModelIndex& parent) const
{
    return errors_.size();
}

int SyncErrorsTableModel::columnCount(const QModelIndex& parent) const
{
    return MAX_COLUMN;
}

void SyncErrorsTableModel::onResize(const QSize &size)
{
    int extra_width = size.width() - kDefaultColumnSum;
    int extra_width_per_column = extra_width / 3;

    repo_name_column_width_ = kRepoNameColumnWidth + extra_width_per_column;
    path_column_width_ = kPathColumnWidth + extra_width_per_column;
    error_column_width_ = kErrorColumnWidth + extra_width_per_column;

    // name_column_width_ should be always larger than kPathColumnWidth
    if (errors_.empty())
        return;

    // printf ("path_column_width_ = %d\n", path_column_width_);
    emit dataChanged(
        index(0, COLUMN_ERROR_STR),
        index(errors_.size() - 1 , COLUMN_ERROR_STR));
}

QVariant SyncErrorsTableModel::data(const QModelIndex & index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    int column = index.column();

    if (role == Qt::TextAlignmentRole)
        return Qt::AlignLeft + Qt::AlignVCenter;

    if (role == Qt::ToolTipRole)
        return tr("Double click to open the library");

    if (role == Qt::SizeHintRole) {
        int h = kDefaultColumnHeight;
        int w = kDefaultColumnWidth;
        switch (column) {
        case COLUMN_REPO_NAME:
            w = repo_name_column_width_;
            break;
        case COLUMN_PATH:
            w = path_column_width_;
            break;
        case COLUMN_ERROR_STR:
            w = error_column_width_;
            break;
        case COLUMN_TIMESTAMP:
            w = kTimestampColumnWidth;
            break;
        default:
            break;
        }
        return QSize(w, h);
    }

    if (role != Qt::DisplayRole) {
        return QVariant();
    }

    const SyncError &error = errors_[index.row()];

    if (column == COLUMN_REPO_NAME) {
        return error.repo_name;
    } else if (column == COLUMN_PATH) {
        return QDir::toNativeSeparators(error.path);
    } else if (column == COLUMN_ERROR_STR) {
        return error.error_str;
    } else if (column == COLUMN_TIMESTAMP) {
        return error.readable_time_stamp;
    }

    return QVariant();
}

QVariant SyncErrorsTableModel::headerData(int section,
                                          Qt::Orientation orientation,
                                          int role) const
{
    if (orientation == Qt::Vertical) {
        return QVariant();
    }

    if (role == Qt::TextAlignmentRole)
        return Qt::AlignLeft + Qt::AlignVCenter;

    if (role != Qt::DisplayRole)
        return QVariant();

    if (section == COLUMN_REPO_NAME) {
        return tr("Library");
    } else if (section == COLUMN_PATH) {
        return tr("Path");
    } else if (section == COLUMN_ERROR_STR) {
        return tr("Error");
    } else if (section == COLUMN_TIMESTAMP) {
        return tr("Time");
    }


    return QVariant();
}
