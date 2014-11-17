#include <QtGui>

#include "utils/utils.h"
#include "utils/file-utils.h"
#include "seaf-dirent.h"

#include "file-table.h"
#include "file-browser-dialog.h"
#include "data-mgr.h"

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
      repo_(repo),
      parent_(parent)
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

    setupContextMenu();
}
void FileTableView::setupContextMenu()
{
    FileBrowserDialog *dialog = static_cast<FileBrowserDialog*>(parent_);

    context_menu_ = new QMenu(this);
    download_action_ = new QAction(tr("&Open"), this);
    download_action_->setIcon(QIcon(":images/filebrowser/download.png"));
    connect(download_action_, SIGNAL(triggered()),
            this, SLOT(onOpen()));

    QAction *rename_action_ = new QAction(tr("&Rename"), this);
    connect(rename_action_, SIGNAL(triggered()),
            this, SLOT(onRename()));

    QAction *remove_action_ = new QAction(tr("&Delete"), this);
    connect(remove_action_, SIGNAL(triggered()),
            this, SLOT(onRemove()));

    QAction *share_action_ = new QAction(tr("&Generate Share Link"), this);
    connect(share_action_, SIGNAL(triggered()),
            this, SLOT(onShare()));

    update_action_ = new QAction(tr("&Update"), this);
    connect(update_action_, SIGNAL(triggered()), this, SLOT(onUpdate()));

    context_menu_->setDefaultAction(download_action_);
    context_menu_->addAction(download_action_);
    context_menu_->addAction(share_action_);
    context_menu_->addSeparator();
    context_menu_->addAction(rename_action_);
    context_menu_->addAction(remove_action_);
    context_menu_->addSeparator();
    context_menu_->addAction(update_action_);

    download_action_->setIconVisibleInMenu(false);
    dialog->upload_action_->setIconVisibleInMenu(false);
}
void FileTableView::contextMenuEvent(QContextMenuEvent *event)
{
    QPoint pos = event->pos();
    int row = rowAt(pos.y());
    if (row == -1) {
        return;
    }

    FileTableModel *model = (FileTableModel *)this->model();

    item_.reset(new SeafDirent(*model->direntAt(row)));

    if (item_ == NULL)
        return;
    if (item_->isDir()) {
        update_action_->setVisible(false);
        download_action_->setText(tr("&Open"));
        download_action_->setIcon(QIcon(":images/filebrowser/open-folder.png"));
    } else {
        update_action_->setVisible(true);
        download_action_->setText(tr("&Download"));
        download_action_->setIcon(QIcon(":images/filebrowser/download.png"));
    }

    pos = viewport()->mapToGlobal(pos);
    context_menu_->exec(pos);
}

void FileTableView::onItemDoubleClicked(const QModelIndex& index)
{
    FileTableModel *model = (FileTableModel *)this->model();
    const SeafDirent *dirent = model->direntAt(index.row());

    if (dirent == NULL)
        return;

    emit direntClicked(*dirent);
}

void FileTableView::onOpen()
{
    if (item_ == NULL)
        return;

    emit direntClicked(*item_);
}

void FileTableView::onRename()
{
    if (item_ == NULL)
        return;

    emit direntRename(*item_);
}

void FileTableView::onRemove()
{
    if (item_ == NULL)
        return;

    emit direntRemove(*item_);
}

void FileTableView::onShare()
{
    if (item_ == NULL)
        return;
    emit direntShare(*item_);
}

void FileTableView::onUpdate()
{
    if (item_ == NULL)
        return;
    emit direntUpdate(*item_);
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

    event->accept();

    emit dropFile(file_name);
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
          qsize.setWidth(kColumnIconSize + kColumnIconAlign / 2);
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

const SeafDirent* FileTableModel::direntAt(int index) const
{
    if (index > dirents_.size()) {
        return NULL;
    }

    return &dirents_[index];
}

void FileTableModel::removeItemNamed(const QString &name)
{
    int j = 0;
    for (QList<SeafDirent>::iterator i = dirents_.begin(); i != dirents_.end() ; i++, j++)
        if (i->name == name) {
            dirents_.erase(i);
            emit dataChanged(index(j, 0),
                index(dirents_.size()-1, FILE_MAX_COLUMN - 1));
            break;
        }
}

void FileTableModel::renameItemNamed(const QString &name, const QString &new_name)
{
    for (int i = 0; i != dirents_.size() ; i++)
        if (dirents_[i].name == name) {
            dirents_[i].name = new_name;

            emit dataChanged(index(i, 0),
                index(i , FILE_MAX_COLUMN - 1));

            break;
        }
}

void FileTableModel::onResize(const QSize &size)
{
    name_column_width_ = size.width() - kDefaultColumnSum;
    // name_column_width_ should be always larger than kFileNameColumnWidth
    emit dataChanged(index(0, FILE_COLUMN_NAME),
                     index(dirents_.size()-1 , FILE_COLUMN_NAME));
}
