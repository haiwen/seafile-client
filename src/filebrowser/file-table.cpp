#include <QtGui>

#include "utils/utils.h"
#include "seaf-dirent.h"

#include "file-table.h"

namespace {

enum {
    COLUMN_NAME = 0,
    COLUMN_SIZE,
    COLUMN_MTIME,
    MAX_COLUMN,
};

} // namespace

FileTableView::FileTableView(const ServerRepo& repo, QWidget *parent)
    : QTableView(parent),
      repo_(repo)
{
    horizontalHeader()->setResizeMode(QHeaderView::Stretch);

    connect(this, SIGNAL(doubleClicked(const QModelIndex&)),
            this, SLOT(onItemDoubleClicked(const QModelIndex&)));
}

void FileTableView::onItemDoubleClicked(const QModelIndex& index)
{
    if (index.column() != COLUMN_NAME) {
        return;
    }
    FileTableModel *model = (FileTableModel *)this->model();
    const SeafDirent dirent = model->direntAt(index.row());

    emit direntClicked(dirent);
}

FileTableModel::FileTableModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

void FileTableModel::setDirents(const std::vector<SeafDirent>& dirents)
{
    dirents_ = dirents;
    reset();
}

int FileTableModel::rowCount(const QModelIndex& parent) const
{
    return dirents_.size();
}

int FileTableModel::columnCount(const QModelIndex& parent) const
{
    return MAX_COLUMN;
}

QVariant FileTableModel::data(const QModelIndex & index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    if (role != Qt::DisplayRole) {
        return QVariant();
    }

    const SeafDirent& dirent = dirents_[index.row()];

    int column = index.column();

    if (column == COLUMN_NAME) {
        return dirent.name;
    } else if (column == COLUMN_SIZE) {
        return ::readableFileSize(dirent.size);
    } else if (column == COLUMN_MTIME) {
        return ::translateCommitTime(dirent.mtime);
    }

    return QVariant();
}

QVariant FileTableModel::headerData(int section,
                                    Qt::Orientation orientation,
                                    int role) const
{
    if (orientation == Qt::Vertical || role != Qt::DisplayRole) {
        return QVariant();
    }

    switch (section) {
    case COLUMN_NAME:
        return tr("Name");
    case COLUMN_SIZE:
        return tr("Size");
    case COLUMN_MTIME:
        return tr("Last Modified");
    }

    return QVariant();
}

const SeafDirent FileTableModel::direntAt(int index) const
{
    if (index > dirents_.size()) {
        return SeafDirent();
    }

    return dirents_[index];
}
