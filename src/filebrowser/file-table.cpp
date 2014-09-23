#include "file-table.h"

#include <QtGui>
#include <QApplication>

#include "utils/utils.h"
#include "utils/file-utils.h"
#include "seaf-dirent.h"

const int kFileNameColumnWidth = 300;
const int kDefaultColumnWidth = 160;
const int kDefaultColumnHeight = 36;

FileTableModel::FileTableModel(QObject *parent)
    : QAbstractTableModel(parent),
      selected_dirent_(NULL),
      curr_hovered_(-1) // -1 is a publicly-known magic number
{
    dirents_ = QList<SeafDirent>();
}

FileTableModel::FileTableModel(const QList<SeafDirent>& dirents,
                               QObject *parent)
    : QAbstractTableModel(parent)
{
    dirents_ = dirents;
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

    const int column = index.column();
    const int row = index.row();
    const SeafDirent& dirent = dirents_[row];

    if (role == Qt::DecorationRole && column == FILE_COLUMN_ICON) {
        return (dirent.isDir() ?
            QApplication::style()->standardIcon(QStyle::SP_DirIcon) :
            QIcon(getIconByFileName(dirent.name))).
          pixmap(kDefaultColumnHeight, kDefaultColumnHeight);
    }

    if (role == Qt::TextAlignmentRole && column == FILE_COLUMN_NAME)
        return Qt::AlignLeft + Qt::AlignVCenter;

    if (role == Qt::BackgroundRole && row == curr_hovered_)
        return QColor(200, 200, 220, 255);

    if (role == Qt::SizeHintRole) {
        QSize qsize(kDefaultColumnWidth, kDefaultColumnHeight);
        switch (column) {
        case FILE_COLUMN_ICON:
          qsize.setWidth(kDefaultColumnHeight);
          break;
        case FILE_COLUMN_NAME:
          qsize.setWidth(kFileNameColumnWidth);
          break;
        case FILE_COLUMN_SIZE:
          break;
        case FILE_COLUMN_MTIME:
          break;
        case FILE_COLUMN_KIND:
          break;
        default:
          break;
        }
        return qsize;

    }

    if (role != Qt::DisplayRole) {
        return QVariant();
    }

    switch (column) {
    case FILE_COLUMN_NAME:
      return dirent.name;
    case FILE_COLUMN_SIZE:
      if (dirent.isDir())
        return "";
      return ::readableFileSize(dirent.size);
    case FILE_COLUMN_MTIME:
      return ::translateCommitTime(dirent.mtime);
    case FILE_COLUMN_KIND:
      //TODO: mime file information
      return dirent.isDir() ?
        tr("Folder") :
        tr("Document");
    default:
      return QVariant();
    }
}

QVariant FileTableModel::headerData(int section,
                                    Qt::Orientation orientation,
                                    int role) const
{
    if (orientation == Qt::Vertical)
        return QVariant();

    if (role == Qt::TextAlignmentRole)
        return Qt::AlignLeft + Qt::AlignVCenter;

    if (role != Qt::DisplayRole)
        return QVariant();

    switch (section) {
    case FILE_COLUMN_ICON:
        return "";
    case FILE_COLUMN_NAME:
        return tr("Name");
    case FILE_COLUMN_SIZE:
        return tr("Size");
    case FILE_COLUMN_MTIME:
        return tr("Last Modified");
    case FILE_COLUMN_KIND:
        return tr("Kind");
    default:
        return QVariant();
    }

}

const SeafDirent FileTableModel::direntAt(int index) const
{
    if (index > dirents_.size()) {
        return SeafDirent();
    }

    return dirents_[index];
}

void FileTableModel::setMouseOver(const int row)
{
    const int curr_hovered = curr_hovered_;
    if (curr_hovered_ == row)
        return;
    curr_hovered_ = row;

    if (curr_hovered != -1)
        emit dataChanged(index(curr_hovered, 0),
                         index(curr_hovered, FILE_MAX_COLUMN-1));
    if (curr_hovered_ != -1)
        emit dataChanged(index(curr_hovered_, 0),
                         index(curr_hovered_, FILE_MAX_COLUMN-1));
}

const SeafDirent *FileTableModel::selectedDirent() const
{
    return selected_dirent_;
}

Qt::DropActions FileTableModel::supportedDropActions() const
{
    return Qt::CopyAction | Qt::MoveAction;
}

void FileTableModel::onSelectionChanged(const int row)
{
    if (row != -1) {
        selected_dirent_ = &dirents_[row];
        emit dataChanged(index(row, 0), index(row, FILE_MAX_COLUMN-1));
    }
    else
        selected_dirent_ = NULL;
    return;
}
