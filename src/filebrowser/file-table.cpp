#include <QtGui>
#include <QTimer>

#include "utils/utils.h"
#include "utils/file-utils.h"
#include "seaf-dirent.h"
#include "utils/utils-mac.h"

#include "file-browser-dialog.h"
#include "data-mgr.h"
#include "transfer-mgr.h"
#include "tasks.h"

#include "file-table.h"

namespace {

enum {
    FILE_COLUMN_ICON = 0,
    FILE_COLUMN_NAME,
    FILE_COLUMN_MTIME,
    FILE_COLUMN_SIZE,
    FILE_COLUMN_KIND,
    FILE_COLUMN_PROGRESS,
    FILE_MAX_COLUMN
};

const int kDefaultColumnWidth = 120;
const int kDefaultColumnHeight = 40;
const int kColumnIconSize = 28;
const int kColumnIconAlign = 8;
const int kFileNameColumnWidth = 200;
const int kDefaultColumnSum = kFileNameColumnWidth + kDefaultColumnWidth * 3 + kColumnIconSize + kColumnIconAlign;

const int kRefreshProgressInterval = 1000;

const QColor kSelectedItemBackgroundcColor("#F9E0C7");
const QColor kItemBackgroundColor("white");
const QColor kItemBottomBorderColor("#f3f3f3");
const QColor kItemColor("black");
const QString kProgressBarStyle("QProgressBar "
        "{ border: 1px solid grey; border-radius: 2px; } "
        "QProgressBar::chunk { background-color: #f0f0f0; width: 20px; }");

} // namespace

FileTableViewDelegate::FileTableViewDelegate(QObject *parent)
  : QStyledItemDelegate(parent) {
}

void FileTableViewDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    const FileTableModel *model = static_cast<const FileTableModel*>(index.model());

    // draw item's background
    painter->save();
    if (option.state & QStyle::State_Selected)
        painter->fillRect(option.rect, kSelectedItemBackgroundcColor);
    else
        painter->fillRect(option.rect, kItemBackgroundColor);
    painter->restore();

    static const QPen borderPen(kItemBottomBorderColor, 1);
    // draw item's border for the first row only
    if (index.row() == 0) {
        painter->save();
        painter->setPen(borderPen);
        painter->drawLine(option.rect.topLeft(), option.rect.topRight());
        painter->restore();
    }

    // draw item's border bottom
    painter->save();
    painter->setPen(borderPen);
    painter->drawLine(option.rect.bottomLeft(), option.rect.bottomRight());
    painter->restore();

    // draw item
    QSize size = model->data(index, Qt::SizeHintRole).value<QSize>();
    switch (index.column()) {
    case FILE_COLUMN_ICON:
    {
        QPixmap pixmap = model->data(index, Qt::DecorationRole).value<QPixmap>();
        int alignX = (size.width() - pixmap.width()); // AlignRight
        int alignY = (size.height() - pixmap.height()) / 2; //AlignVCenter
        painter->save();
        painter->drawPixmap(option.rect.topLeft() + QPoint(alignX, alignY - 2), pixmap);
        painter->restore();
    }
        break;
    case FILE_COLUMN_SIZE:
    {
        // if we has progress, draw the progress bar
        QString text_progress = model->data(model->index(index.row(), FILE_COLUMN_PROGRESS), Qt::DisplayRole).value<QString>();
        if (!text_progress.isEmpty()) {
            // get the progress value from the Model
            if (text_progress.endsWith('%'))
                text_progress.resize(text_progress.size() - 1);
            const int progress = text_progress.toInt();

            // Customize style using style-sheet..
            QProgressBar progressBar;
            progressBar.resize(QSize(size.width() - 10, size.height() / 2 - 4));
            progressBar.setMinimum(0);
            progressBar.setMaximum(100);
            progressBar.setValue(progress);
            progressBar.setAlignment(Qt::AlignCenter);
            progressBar.setStyleSheet(kProgressBarStyle);
            painter->save();
            painter->translate(option.rect.topLeft() + QPoint(0, size.height() / 4 - 1));
            progressBar.render(painter);
            painter->restore();
            break;
        }
        // else, draw the text only
    }
    case FILE_COLUMN_NAME:
    case FILE_COLUMN_MTIME:
    case FILE_COLUMN_KIND:
    {
        QString text = model->data(index, Qt::DisplayRole).value<QString>();
        QFont font = model->data(index, Qt::FontRole).value<QFont>();
        QRect rect(option.rect.topLeft() + QPoint(4, -2), size - QSize(10, 0));
        painter->setPen(kItemColor);
        painter->setFont(font);
        painter->drawText(rect, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine, text);
    }
        break;
    case FILE_COLUMN_PROGRESS:
        // don't do anything
        break;
    default:
        // never reached here
        // QStyledItemDelegate::paint(painter, option, index);
        qDebug() << "invalid item (row)";
        break;
    }
}

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

    setGridStyle(Qt::NoPen);
    setShowGrid(false);
    setContentsMargins(0, 0, 0, 0);
    setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::ExtendedSelection);

    setMouseTracking(true);
    setAcceptDrops(true);
    setDragDropMode(QAbstractItemView::DropOnly);
    setItemDelegate(new FileTableViewDelegate(this));

    connect(this, SIGNAL(doubleClicked(const QModelIndex&)),
            this, SLOT(onItemDoubleClicked(const QModelIndex&)));

    setupContextMenu();
}

void FileTableView::unselectItemNamed(const QString &name)
{
    FileTableModel *model = static_cast<FileTableModel *>(this->model());
    QItemSelectionModel *selections = this->selectionModel();
    QModelIndexList selected = selections->selectedRows();
    for (int i = 0; i < selected.size(); i++) {
        const SeafDirent *dirent = model->direntAt(selected[i].row());
        if (dirent->name == name)
            selections->select(selected[i], QItemSelectionModel::Deselect | QItemSelectionModel::Current);
    }
}

void FileTableView::setModel(QAbstractItemModel *model)
{
    QTableView::setModel(model);
    setColumnHidden(FILE_COLUMN_PROGRESS, true);
}

void FileTableView::setupContextMenu()
{
    context_menu_ = new QMenu(this);
    download_action_ = new QAction(tr("&Open"), this);
    download_action_->setIcon(QIcon(":images/filebrowser/download.png"));
    connect(download_action_, SIGNAL(triggered()),
            this, SLOT(onOpen()));

    rename_action_ = new QAction(tr("&Rename"), this);
    connect(rename_action_, SIGNAL(triggered()),
            this, SLOT(onRename()));

    remove_action_ = new QAction(tr("&Delete"), this);
    connect(remove_action_, SIGNAL(triggered()),
            this, SLOT(onRemove()));

    share_action_ = new QAction(tr("&Generate Share Link"), this);
    connect(share_action_, SIGNAL(triggered()),
            this, SLOT(onShare()));

    update_action_ = new QAction(tr("&Update"), this);
    connect(update_action_, SIGNAL(triggered()), this, SLOT(onUpdate()));

    cancel_download_action_ = new QAction(tr("&Cancel Download"), this);
    connect(cancel_download_action_, SIGNAL(triggered()),
            this, SLOT(onCancelDownload()));

    context_menu_->setDefaultAction(download_action_);
    context_menu_->addAction(download_action_);
    context_menu_->addAction(share_action_);
    context_menu_->addSeparator();
    context_menu_->addAction(rename_action_);
    context_menu_->addAction(remove_action_);
    context_menu_->addSeparator();
    context_menu_->addAction(update_action_);
    context_menu_->addAction(cancel_download_action_);

    download_action_->setIconVisibleInMenu(false);
}

void FileTableView::contextMenuEvent(QContextMenuEvent *event)
{
    QPoint pos = event->pos();
    int row = rowAt(pos.y());
    if (row == -1) {
        return;
    }

    FileTableModel *model = static_cast<FileTableModel *>(this->model());
    QItemSelectionModel *selections = this->selectionModel();
    QModelIndexList selected = selections->selectedRows();

    // find if the item is in the selection
    int i;
    for (i = 0; i < selected.size(); i++)
    {
        if (row == selected[i].row())
            break;
    }
    // if the item is and it is a multi-selection
    if (i != selected.size() && selected.size() != 1) {
        item_.reset(NULL);

        download_action_->setVisible(true);

        download_action_->setVisible(true);
        download_action_->setText(tr("&Download"));
        download_action_->setIcon(QIcon(":images/filebrowser/download.png"));
        rename_action_->setVisible(false);
        share_action_->setVisible(false);
        update_action_->setVisible(false);
        cancel_download_action_->setVisible(false);
        pos = viewport()->mapToGlobal(pos);
        context_menu_->exec(pos);
        return;
    }

    // if the item is not
    rename_action_->setVisible(true);
    share_action_->setVisible(true);
    update_action_->setVisible(true);
    cancel_download_action_->setVisible(true);

    const SeafDirent *dirent = model->direntAt(row);
    item_.reset(new SeafDirent(*dirent));

    download_action_->setVisible(true);
    cancel_download_action_->setVisible(false);
    if (item_->isDir()) {
        update_action_->setVisible(false);
        download_action_->setText(tr("&Open"));
        download_action_->setIcon(QIcon(":images/filebrowser/open-folder.png"));
    } else {
        update_action_->setVisible(true);
        download_action_->setText(tr("D&ownload"));
        download_action_->setIcon(QIcon(":images/filebrowser/download.png"));

        FileBrowserDialog *dialog = (FileBrowserDialog *)parent_;
        if (TransferManager::instance()->hasDownloadTask(dialog->repo_.id,
            ::pathJoin(dialog->current_path_, dirent->name))) {
            cancel_download_action_->setVisible(true);
            download_action_->setVisible(false);
        }
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
    if (item_ == NULL) {
        FileTableModel *model = static_cast<FileTableModel *>(this->model());
        QItemSelectionModel *selections = this->selectionModel();
        QModelIndexList selected = selections->selectedRows();
        const SeafDirent *dirent;
        for (int i = 0; i < selected.size(); i++) {
            dirent = model->direntAt(selected[i].row());
            if (dirent->isDir())
                continue;

            emit direntClicked(*dirent);
        }
        return;
    }

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
    if (item_ == NULL) {
        FileTableModel *model = static_cast<FileTableModel *>(this->model());
        QItemSelectionModel *selections = this->selectionModel();
        QModelIndexList selected = selections->selectedRows();
        QList<const SeafDirent*> dirents;
        for (int i = 0; i < selected.size(); i++) {
            dirents.push_back(model->direntAt(selected[i].row()));
        }
        emit direntRemove(dirents);
        return;
    }

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

void FileTableView::onCancelDownload()
{
    if (item_ == NULL)
        return;
    emit cancelDownload(*item_);
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
#ifdef Q_WS_MAC
    if (file_name.startsWith("/.file/id="))
        file_name = __mac_get_path_from_fileId_url("file://" + file_name);
#endif

    if(file_name.isEmpty())
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
    task_progress_timer_ = new QTimer(this);
    connect(task_progress_timer_, SIGNAL(timeout()),
            this, SLOT(updateDownloadInfo()));
    task_progress_timer_->start(kRefreshProgressInterval);
}

void FileTableModel::setDirents(const QList<SeafDirent>& dirents)
{
    dirents_ = dirents;
    progresses_.clear();
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

    if (role == Qt::SizeHintRole) {
        QSize qsize(kDefaultColumnWidth, kDefaultColumnHeight);
        switch (column) {
        case FILE_COLUMN_ICON:
            qsize.setWidth(kColumnIconSize + kColumnIconAlign / 2 + 2);
            break;
        case FILE_COLUMN_NAME:
            qsize.setWidth(name_column_width_);
            break;
        case FILE_COLUMN_PROGRESS:
        case FILE_COLUMN_SIZE:
        case FILE_COLUMN_MTIME:
        case FILE_COLUMN_KIND:
        default:
            break;
        }
        return qsize;
    }

    if (role != Qt::DisplayRole) {
        return QVariant();
    }

    // DisplayRole

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
        return dirent.isDir() ?  tr("Folder") : tr("Document");
    case FILE_COLUMN_PROGRESS:
        return getTransferProgress(dirent);
    default:
        return QVariant();
    }
}

QString FileTableModel::getTransferProgress(const SeafDirent& dirent) const
{
    return progresses_[dirent.name];
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

void FileTableModel::replaceItem(const QString &name, const SeafDirent &dirent)
{
    for (int i = 0; i != dirents_.size() ; i++)
        if (dirents_[i].name == name) {
            dirents_[i] = dirent;

            emit dataChanged(index(i, 0), index(i , FILE_MAX_COLUMN - 1));

            break;
        }
}

void FileTableModel::insertItem(const SeafDirent &dirent)
{
    dirents_.insert(0, dirent);
    emit layoutChanged();
}

void FileTableModel::appendItem(const SeafDirent &dirent)
{
    dirents_.push_back(dirent);
    emit layoutChanged();
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

            emit dataChanged(index(i, 0), index(i , FILE_MAX_COLUMN - 1));

            break;
        }
}

void FileTableModel::onResize(const QSize &size)
{
    name_column_width_ = size.width() - kDefaultColumnSum + kFileNameColumnWidth;
    // name_column_width_ should be always larger than kFileNameColumnWidth
    emit dataChanged(index(0, FILE_COLUMN_NAME),
                     index(dirents_.size()-1 , FILE_COLUMN_NAME));
}

void FileTableModel::updateDownloadInfo()
{
    FileBrowserDialog *dialog = (FileBrowserDialog *)(QObject::parent());
    QList<FileDownloadTask*> tasks= TransferManager::instance()->getDownloadTasks(
        dialog->repo_.id, dialog->current_path_);

    progresses_.clear();

    foreach (const FileDownloadTask *task, tasks) {
        QString progress = task->progress().toString();
        progresses_[::getBaseName(task->path())] = progress;
    }

    emit dataChanged(index(0, FILE_COLUMN_SIZE),
                     index(dirents_.size() - 1 , FILE_COLUMN_SIZE));
}
