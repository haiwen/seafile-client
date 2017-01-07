#include <QtGlobal>

#include <QtWidgets>
#include <QTableView>
#include <QTimer>

#include "utils/utils.h"
#include "seafile-applet.h"
#include "rpc/rpc-client.h"
#include "rpc/sync-error.h"
#include "sync-errors-dialog.h"

namespace {

const int kUpdateErrorsInterval = 3000;

enum {
    INDEX_EMPTY_VIEW = 0,
    INDEX_ERRORS_VIEW
};

enum {
    COLUMN_REPO_NAME = 0,
    COLUMN_PATH,
    COLUMN_ERROR_STR,
    COLUMN_TIMESTAMP,
    MAX_COLUMN,
};


} // namespace


SyncErrorsDialog::SyncErrorsDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUi(this);

    setWindowTitle(tr("File Sync Errors"));
    setWindowIcon(QIcon(":/images/seafile.png"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    setMinimumSize(QSize(600, 300));

    createEmptyView();

    table_ = new SyncErrorsTableView;
    model_ = new SyncErrorsTableModel(this);
    table_->setModel(model_);

    stack_ = new QStackedWidget;
    stack_->insertWidget(INDEX_EMPTY_VIEW, empty_view_);
    stack_->insertWidget(INDEX_ERRORS_VIEW, table_);

    QVBoxLayout *vlayout = (QVBoxLayout *)layout();
    vlayout->insertWidget(0, stack_);

    onModelReset();
    connect(model_, SIGNAL(modelReset()), this, SLOT(onModelReset()));
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
        stack_->setCurrentIndex(INDEX_ERRORS_VIEW);
        //table_->justifyColumnWidth();
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

SyncErrorsTableHeader::SyncErrorsTableHeader(QWidget *parent)
    : QHeaderView(Qt::Horizontal, parent)
{
    sectionResizeMode(QHeaderView::Stretch);
    setHighlightSections(false);
}

SyncErrorsTableView::SyncErrorsTableView(QWidget *parent)
    : QTableView(parent)
{
    setHorizontalHeader(new SyncErrorsTableHeader(this));
    setSelectionBehavior(QAbstractItemView::SelectRows);
    verticalHeader()->hide();

    horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);

    createContextMenu();
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

SyncErrorsTableModel::SyncErrorsTableModel(QObject *parent)
    : QAbstractTableModel(parent)
{
    update_timer_ = new QTimer(this);
    connect(update_timer_, SIGNAL(timeout()), this, SLOT(updateErrors()));
    update_timer_->start(kUpdateErrorsInterval);

    updateErrors();
}

void SyncErrorsTableModel::updateErrors()
{
    std::vector<SyncError> errors;
    int ret = seafApplet->rpcClient()->getSyncErrors(&errors);
    if (ret < 0) {
        qDebug("failed to get sync errors");
        return;
    }

    beginResetModel();
    if (errors_.size() != errors.size()) {
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
    endResetModel();
}

int SyncErrorsTableModel::rowCount(const QModelIndex& parent) const
{
    return errors_.size();
}

int SyncErrorsTableModel::columnCount(const QModelIndex& parent) const
{
    return MAX_COLUMN;
}

QVariant SyncErrorsTableModel::data(const QModelIndex & index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    if (role != Qt::DisplayRole) {
        return QVariant();
    }

    const SyncError &error = errors_[index.row()];

    int column = index.column();

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

    if (section == COLUMN_REPO_NAME) {
        if (role == Qt::DisplayRole) {
            return tr("Library");
        }
        //else if (role == Qt::DecorationRole) {
        //    return awesome->icon(icon_cloud);
        //}
    } else if (section == COLUMN_PATH) {
        if (role == Qt::DisplayRole) {
            return tr("Path");
        }
        //else if (role == Qt::DecorationRole) {
        //   return awesome->icon(icon_folder_close_alt);
        //}
    }


    return QVariant();
}
