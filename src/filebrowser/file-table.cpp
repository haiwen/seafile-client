#include <QtGui>
#include <cassert>

#include "utils/utils.h"
#include "seaf-dirent.h"
#include "file-table.h"

FileTableModel::FileTableModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

void FileTableModel::setDirents(const QList<SeafDirent>& dirents)
{
    dirents_ = dirents;
    this->reset();
}

QList<SeafDirent> FileTableModel::dirents()
{
    return dirents_;
}

int FileTableModel::rowCount(const QModelIndex& /* parent */) const
{
    return dirents_.size();
}

int FileTableModel::columnCount(const QModelIndex& /* parent */) const
{
    return FILE_MAX_COLUMN;
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

    if (column == FILE_COLUMN_NAME) {
        return dirent.name;
    } else if (column == FILE_COLUMN_SIZE) {
        return ::readableFileSize(dirent.size);
    } else if (column == FILE_COLUMN_MTIME) {
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
    case FILE_COLUMN_NAME:
        return tr("Name");
    case FILE_COLUMN_SIZE:
        return tr("Size");
    case FILE_COLUMN_MTIME:
        return tr("Last Modified");
    }

    return QVariant();
}

const SeafDirent FileTableModel::direntAt(int index) const
{
    assert(index <= dirents_.size());

    return dirents_[index];
}


Qt::ItemFlags FileTableModel::flags (const QModelIndex & index) const
{
    Qt::ItemFlags flags = QAbstractTableModel::flags(index);

    return flags | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
}
