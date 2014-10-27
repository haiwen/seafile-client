#include <QtGui>

#include "utils/utils.h"
#include "utils/file-utils.h"
#include "seaf-dirent.h"

#include "file-table.h"

namespace {

enum {
    FILE_COLUMN_ICON = 0,
    FILE_COLUMN_NAME,
    FILE_COLUMN_MTIME,
    FILE_COLUMN_SIZE,
    FILE_COLUMN_KIND,
    FILE_MAX_COLUMN
};

const int kDefaultColumnWidth = 120;
const int kDefaultColumnHeight = 40;
const int kColumnIconSize = 28;
const int kColumnIconAlign = 8;
const int kColumnExtraAlign = 40;
const int kDefaultColumnSum = kDefaultColumnWidth * 3 + kColumnIconSize + kColumnIconAlign + kColumnExtraAlign;
const int kFileNameColumnWidth = 200;

} // namespace

FileTableView::FileTableView(const ServerRepo& repo, QWidget *parent)
    : QTableView(parent),
      repo_(repo)
{
    verticalHeader()->hide();
    verticalHeader()->setDefaultSectionSize(36);
    horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
    horizontalHeader()->setStretchLastSection(true);
    horizontalHeader()->setCascadingSectionResizes(true);
    horizontalHeader()->setHighlightSections(false);
    horizontalHeader()->setSortIndicatorShown(false);
    horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    horizontalHeader()->setStyleSheet("background-color: white");

    setGridStyle(Qt::NoPen);
    setShowGrid(false);
    setContentsMargins(0, 0, 0, 0);
    setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);

    setMouseTracking(true);
    setAcceptDrops(true);
    setDragDropMode(QAbstractItemView::DropOnly);

    connect(this, SIGNAL(doubleClicked(const QModelIndex&)),
            this, SLOT(onItemDoubleClicked(const QModelIndex&)));
}

void FileTableView::onItemDoubleClicked(const QModelIndex& index)
{
    FileTableModel *model = (FileTableModel *)this->model();
    const SeafDirent dirent = model->direntAt(index.row());

    emit direntClicked(dirent);
}

void FileTableView::dropEvent(QDropEvent *event)
{
    // only handle external source currently
    if(event->source() != NULL)
        return;

    QList<QUrl> urls = event->mimeData()->urls();

    if(urls.isEmpty())
        return;

    // since we supports processing only one file at a time, skip the rest
    QString file_name = urls.first().toLocalFile();

    if(file_name.isEmpty())
        return;

    if(QFileInfo(file_name).isDir())
        return;

    emit dropFile(file_name);

    event->accept();
}

void FileTableView::dragMoveEvent(QDragMoveEvent *event)
{
    // this is needed
    event->accept();
}

void FileTableView::dragEnterEvent(QDragEnterEvent *event)
{
    // only handle external source currently
    if(event->source() != NULL)
        return;

    // Otherwise it might be a MoveAction which is unacceptable
    event->setDropAction(Qt::CopyAction);

    // trivial check
    if(event->mimeData()->hasFormat("text/uri-list"))
        event->accept();
}

void FileTableView::resizeEvent(QResizeEvent *event)
{
    QTableView::resizeEvent(event);
    static_cast<FileTableModel*>(model())->onResize(event->size());
}

FileTableModel::FileTableModel(QObject *parent)
    : QAbstractTableModel(parent),
    name_column_width_(kFileNameColumnWidth)
{
}

void FileTableModel::setDirents(const QList<SeafDirent>& dirents)
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
            QIcon(":/images/files_v2/file_folder.png") :
            QIcon(getIconByFileNameV2(dirent.name))).
          pixmap(kColumnIconSize, kColumnIconSize);
    }

    if (role == Qt::TextAlignmentRole && column == FILE_COLUMN_ICON)
        return Qt::AlignRight + Qt::AlignVCenter;

    if (role == Qt::TextAlignmentRole && column == FILE_COLUMN_NAME)
        return Qt::AlignLeft + Qt::AlignVCenter;

    if (role == Qt::SizeHintRole) {
        QSize qsize(kDefaultColumnWidth, kDefaultColumnHeight);
        switch (column) {
        case FILE_COLUMN_ICON:
          qsize.setWidth(kColumnIconSize + kColumnIconAlign);
          break;
        case FILE_COLUMN_NAME:
          qsize.setWidth(name_column_width_);
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

    //change color only for file name column
    if (role == Qt::ForegroundRole && column == FILE_COLUMN_NAME)
        return QColor("#e83");

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

void FileTableModel::onResize(const QSize &size)
{
    name_column_width_ = size.width() - kDefaultColumnSum;
    // name_column_width_ should be always larger than kFileNameColumnWidth
    emit dataChanged(index(0, FILE_COLUMN_NAME),
                     index(dirents_.size()-1 , FILE_COLUMN_NAME));
}
