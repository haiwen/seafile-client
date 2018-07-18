#include <QtGlobal>
#include <QtWidgets>
#include <QTimer>

#include "utils/utils.h"
#include "utils/file-utils.h"
#include "utils/paint-utils.h"
#include "seaf-dirent.h"
#include "utils/utils-mac.h"
#include "seafile-applet.h"

#include "file-browser-dialog.h"
#include "data-mgr.h"
#include "transfer-mgr.h"
#include "tasks.h"

#include "file-table.h"

namespace {

enum {
    FILE_COLUMN_NAME = 0,
    FILE_COLUMN_MTIME,
    FILE_COLUMN_SIZE,
    FILE_COLUMN_KIND,
    FILE_COLUMN_PROGRESS,
    FILE_MAX_COLUMN
};

int kMarginLeft = 9;
const int kDefaultColumnWidth = 120;
const int kDefaultColumnHeight = 40;
const int kColumnIconSize = 28;
const int kFileNameColumnWidth = 200;
const int kExtraPadding = 30;
const int kDefaultColumnSum = kFileNameColumnWidth + kDefaultColumnWidth * 3 + kExtraPadding;
const int kLockIconSize = 16;
const int kFileStatusIconSize = 16;
const int kMarginBetweenFileNameAndStatusIcon = 5;

const int kRefreshProgressInterval = 1000;

const QColor kSelectedItemBackgroundcColor("#F9E0C7");
const QColor kItemBackgroundColor("white");
const QColor kItemBottomBorderColor("#ECECEC");
const QColor kFileNameFontColor("black");
const QColor kFontColor("#757575");
const QString kProgressBarStyle("QProgressBar "
        "{ border: 1px solid grey; border-radius: 2px; } "
        "QProgressBar::chunk { background-color: #f0f0f0; width: 1px; }");

enum {
    NOT_LOCKED = 0,
    LOCKED_BY_ME,
    LOCKED_BY_OTHERS
};

const int DirentLockStatusRole = Qt::UserRole + 1;
const int DirentLockOwnerRole = Qt::UserRole + 2;
const int DirentCacheStatusRole = Qt::UserRole + 3;

QIcon getFileStatusIcon(AutoUpdateManager::FileStatus file_status)
{
    QString icon;
    const QString prefix = ":/images/sync/";
    switch (file_status) {
        case AutoUpdateManager::SYNCED:
            icon = "status-done";
            break;
        case AutoUpdateManager::UPLOADING:
            icon = "status-syncing";
            break;
        case AutoUpdateManager::NOT_SYNCED:
            icon = "exclamation";
            break;
    }

    return QIcon(prefix + icon + ".png");
}

} // namespace

FileTableViewDelegate::FileTableViewDelegate(QObject *parent)
  : QStyledItemDelegate(parent) {
}

void FileTableViewDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    const FileTableModel *model = static_cast<const FileTableModel*>(index.model());

    // fix for the last item
    QRect option_rect = option.rect;

    // draw item's background
    painter->save();
    if (option.state & QStyle::State_Selected)
        painter->fillRect(option_rect, kSelectedItemBackgroundcColor);
    else
        painter->fillRect(option_rect, kItemBackgroundColor);
    painter->restore();

    //
    // draw item's border
    //

    // draw item's border for the first row only
    static const QPen borderPen(kItemBottomBorderColor, 1);
    if (index.row() == 0) {
        painter->save();
        painter->setPen(borderPen);
        painter->drawLine(option_rect.topLeft(), option_rect.topRight());
        painter->restore();
    }
    // draw item's border under the bottom
    painter->save();
    painter->setPen(borderPen);
    painter->drawLine(option_rect.bottomLeft(), option_rect.bottomRight());
    painter->restore();

    //
    // draw item
    //

    QSize size = model->data(index, Qt::SizeHintRole).value<QSize>();
    QString text = model->data(index, Qt::DisplayRole).value<QString>();
    switch (index.column()) {
    case FILE_COLUMN_NAME:
    {
        // draw icon
        QPixmap pixmap = model->data(index, Qt::DecorationRole).value<QPixmap>();
        double scale_factor = globalDevicePixelRatio();
        // On Mac OSX (and other HDPI screens) the pixmap would be the 2x
        // version (but the draw rect area is still the same size), so when
        // computing the offsets we need to divide it by the scale factor.
        int icon_width = qMin(kColumnIconSize,
                             int((double)pixmap.width() / (double)scale_factor));
        int icon_height = qMin(size.height(),
                               int((double)pixmap.height() / (double)scale_factor));
        int alignX = (kColumnIconSize - icon_width) / 2;
        int alignY = (size.height() - icon_height) / 2;

#ifdef Q_OS_WIN32
    kMarginLeft = 4;
#endif

        QRect icon_bound_rect(
            option_rect.topLeft() + QPoint(kMarginLeft + alignX, alignY - 2),
            QSize(icon_width, icon_height));

        painter->save();
        painter->drawPixmap(icon_bound_rect, pixmap);
        painter->restore();

        int lock_status = model->data(index, DirentLockStatusRole).toInt();
        if (lock_status != NOT_LOCKED) {
            painter->save();
            QString image = QString(":/images/filebrowser/%1.png").arg(
                lock_status == LOCKED_BY_ME ? "locked-by-me" : "locked");
            QPixmap locked_pixmap = QIcon(image).pixmap(kLockIconSize, kLockIconSize);
            int alignX = (kColumnIconSize / 2) + 3;
            int alignY = (kColumnIconSize / 2) + 2;
            painter->drawPixmap(option_rect.topLeft() + QPoint(kMarginLeft + alignX, alignY), locked_pixmap);
            painter->restore();
        }

        // draw text
        QFont font = model->data(index, Qt::FontRole).value<QFont>();
        QRect rect(option_rect.topLeft() + QPoint(kMarginLeft + 2 * 2 + kColumnIconSize, -2),
                   size - QSize(kColumnIconSize + kMarginLeft, 0));
        painter->setPen(kFileNameFontColor);
        painter->setFont(font);
        painter->drawText(
            rect,
            Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine,
            fitTextToWidth(
                text,
                option.font,
                rect.width() - kMarginBetweenFileNameAndStatusIcon - kFileStatusIconSize - 5));

        QVariant file_status_v = model->data(index, DirentCacheStatusRole);
        if (!file_status_v.isNull()) {
            AutoUpdateManager::FileStatus file_status = (AutoUpdateManager::FileStatus)file_status_v.toInt();
            // printf ("file status is %d\n", (int)file_status);

            QPoint status_icon_pos = option.rect.topRight() - QPoint(kFileStatusIconSize + 3, 0);
            status_icon_pos.setY(option.rect.center().y() - (kFileStatusIconSize / 2));
            QRect status_icon_rect(status_icon_pos, QSize(kFileStatusIconSize, kFileStatusIconSize));

            QPixmap status_icon_pixmap = getFileStatusIcon(file_status).pixmap(status_icon_rect.size());

            painter->save();
            painter->drawPixmap(status_icon_rect, status_icon_pixmap);
            painter->restore();
        }
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
            painter->translate(option_rect.topLeft() + QPoint(0, size.height() / 4 - 1));
            progressBar.render(painter);
            painter->restore();
            break;
        }
        // else, draw the text only
    }
    // FILE_COLUMN_SIZE comes here
        if (!text.isEmpty())
            text = ::readableFileSize(model->data(index, Qt::DisplayRole).value<quint64>());
    // no break, continue
    case FILE_COLUMN_MTIME:
        if (index.column() == FILE_COLUMN_MTIME)
            text = ::translateCommitTime(model->data(index, Qt::DisplayRole).value<quint64>());
    // no break, continue
    case FILE_COLUMN_KIND:
    {
        if (index.column() == FILE_COLUMN_KIND) {
            text = model->data(index, Qt::UserRole).toString();
        }
        QFont font = model->data(index, Qt::FontRole).value<QFont>();
        QRect rect(option_rect.topLeft() + QPoint(9, -2), size - QSize(10, 0));
        painter->save();
        painter->setPen(kFontColor);
        painter->setFont(font);
        painter->drawText(rect, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine, text);
        painter->restore();
    }
    break;
    case FILE_COLUMN_PROGRESS:
    // don't do anything
    break;
    default:
    // never reached here
    // QStyledItemDelegate::paint(painter, option, index);
    qWarning() << "invalid item (row)";
    break;
    }
}

FileTableView::FileTableView(QWidget *parent)
    : QTableView(parent),
      parent_(qobject_cast<FileBrowserDialog*>(parent)),
      source_model_(NULL),
      proxy_model_(NULL)
{
    verticalHeader()->hide();
    verticalHeader()->setDefaultSectionSize(kDefaultColumnHeight);
    horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    horizontalHeader()->setStretchLastSection(true);
    horizontalHeader()->setCascadingSectionResizes(true);
    horizontalHeader()->setHighlightSections(false);
    horizontalHeader()->setSortIndicatorShown(true);
    horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    setGridStyle(Qt::NoPen);
    setShowGrid(false);
    setContentsMargins(0, 0, 0, 0);
    setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::ExtendedSelection);

    setMouseTracking(true);
    setDragDropMode(QAbstractItemView::DropOnly);
    setItemDelegate(new FileTableViewDelegate(this));

    connect(this, SIGNAL(doubleClicked(const QModelIndex&)),
            this, SLOT(onItemDoubleClicked(const QModelIndex&)));

    setupContextMenu();
}

void FileTableView::setModel(QAbstractItemModel *model)
{
    source_model_ = qobject_cast<FileTableModel*>(model);
    if (!source_model_)
        return;
    proxy_model_ = new FileTableSortFilterProxyModel(source_model_);
    proxy_model_->setSourceModel(source_model_);
    QTableView::setModel(proxy_model_);

    setColumnHidden(FILE_COLUMN_PROGRESS, true);
    connect(model, SIGNAL(modelAboutToBeReset()), this, SLOT(onAboutToReset()));
    setSortingEnabled(true);

    // set default sort by folder
    sortByColumn(FILE_COLUMN_NAME, Qt::AscendingOrder);
}

const SeafDirent *FileTableView::getSelectedItemFromSource()
{
    QItemSelectionModel *selections = this->selectionModel();
    QModelIndexList selected = selections->selectedRows();
    if (selected.size() == 1)
        return source_model_->direntAt(proxy_model_->mapToSource(selected.front()).row());
    return NULL;
}

QList<const SeafDirent *> FileTableView::getSelectedItemsFromSource()
{
    QList<const SeafDirent *> results;
    QItemSelectionModel *selections = this->selectionModel();
    QModelIndexList selected = selections->selectedRows();
    results.reserve(selected.size());
    for (int i = 0; i != selected.size() ; i++) {
        results.push_back(source_model_->direntAt(proxy_model_->mapToSource(selected[i]).row()));
    }
    return results;
}

void FileTableView::setupContextMenu()
{
    context_menu_ = new QMenu(this);
    connect(parent_, SIGNAL(aboutToClose()),
            context_menu_, SLOT(close()));

    retry_upload_cached_file_action_ = new QAction(tr("Retry Upload"), this);
    connect(retry_upload_cached_file_action_, SIGNAL(triggered()),
            this, SLOT(onRetryUploadCachedFile()));
    delete_local_version_action_ = new QAction(tr("Delete Local Version"), this);
    connect(delete_local_version_action_, SIGNAL(triggered()),
            this, SLOT(onDeleteLocalVersion()));
    local_version_saveas_action_ = new QAction(tr("Local Version Save As..."), this);
    connect(local_version_saveas_action_, SIGNAL(triggered()),
            this, SLOT(onLocalVersionSaveAs()));
    saveas_action_ = new QAction(tr("&Save As..."), this);
    connect(saveas_action_, SIGNAL(triggered()),
            this, SLOT(onSaveAs()));
    saveas_action_->setShortcut(Qt::ALT + Qt::Key_S);

    lock_action_ = new QAction(tr("&Lock"), this);
    connect(lock_action_, SIGNAL(triggered()), this, SLOT(onLock()));
    lock_action_->setShortcut(Qt::ALT + Qt::Key_L);
    if (!parent_->account_.isAtLeastProVersion(4, 2, 0)) {
        lock_action_->setEnabled(false);
        lock_action_->setToolTip(tr("This feature is available in pro version only\n"));
    }

    rename_action_ = new QAction(tr("&Rename"), this);
    connect(rename_action_, SIGNAL(triggered()),
            this, SLOT(onRename()));
    rename_action_->setShortcut(Qt::ALT + Qt::Key_R);

    remove_action_ = new QAction(tr("&Delete"), this);
    connect(remove_action_, SIGNAL(triggered()),
            this, SLOT(onRemove()));
    // remove_action_->setShortcut(QKeySequence::Delete);

    share_action_ = new QAction(tr("&Generate %1 Download Link").arg(getBrand()), this);
    connect(share_action_, SIGNAL(triggered()),
            this, SLOT(onShare()));
    share_action_->setShortcut(Qt::ALT + Qt::Key_G);

    share_to_user_action_ = new QAction(tr("Share to User"), this);
    connect(share_to_user_action_, SIGNAL(triggered()),
            this, SLOT(onShareToUser()));

    share_to_group_action_ = new QAction(tr("Share to Group"), this);
    connect(share_to_group_action_, SIGNAL(triggered()),
            this, SLOT(onShareToGroup()));

    share_seafile_action_ = new QAction(tr("G&enerate %1 Internal Link").arg(getBrand()), this);
    connect(share_seafile_action_, SIGNAL(triggered()),
            this, SLOT(onShareSeafile()));
    share_seafile_action_->setShortcut(Qt::ALT + Qt::Key_E);

    if (parent_->repo_.encrypted) {
        share_action_->setEnabled(false);
        share_to_user_action_->setEnabled(false);
        share_to_group_action_->setEnabled(false);
    }

    update_action_ = new QAction(tr("&Update"), this);
    connect(update_action_, SIGNAL(triggered()), this, SLOT(onUpdate()));
    update_action_->setShortcut(Qt::ALT + Qt::Key_U);

    copy_action_ = new QAction(tr("&Copy"), this);
    connect(copy_action_, SIGNAL(triggered()), this, SLOT(onCopy()));
    copy_action_->setShortcut(QKeySequence::Copy);

    move_action_ = new QAction(tr("Cu&t"), this);
    connect(move_action_, SIGNAL(triggered()), this, SLOT(onMove()));
    move_action_->setShortcut(QKeySequence::Cut);

    paste_action_ = new QAction(tr("&Paste"), this);
    connect(paste_action_, SIGNAL(triggered()), this, SIGNAL(direntPaste()));
    paste_action_->setShortcut(QKeySequence::Paste);

    if (parent_->current_readonly_) {
        move_action_->setEnabled(false);
        paste_action_->setEnabled(false);
    }

    cancel_download_action_ = new QAction(tr("Canc&el Download"), this);
    connect(cancel_download_action_, SIGNAL(triggered()),
            this, SLOT(onCancelDownload()));
    cancel_download_action_->setShortcut(Qt::ALT + Qt::Key_C);

    sync_subdirectory_action_ = new QAction(tr("&Sync this folder"), this);
    connect(sync_subdirectory_action_, SIGNAL(triggered()),
            this, SLOT(onSyncSubdirectory()));
    // sync_subdirectory_action_->setShortcut(Qt::ALT + Qt::Key_S);
    if (!parent_->account_.isAtLeastVersion(4, 1, 0)) {
        sync_subdirectory_action_->setEnabled(false);
        sync_subdirectory_action_->setToolTip(tr("This feature is available in pro version only\n"));
    }

    open_local_cache_folder_action_ = new QAction(tr("Open Local Cache Folder"), this);
    connect(open_local_cache_folder_action_, SIGNAL(triggered()),
                this, SLOT(onOpenLocalCacheFolder()));

    context_menu_->addAction(retry_upload_cached_file_action_);
    context_menu_->addAction(delete_local_version_action_);
    context_menu_->addAction(local_version_saveas_action_);
    context_menu_->addSeparator();
    context_menu_->addAction(saveas_action_);
    context_menu_->addAction(share_action_);
    context_menu_->addAction(share_seafile_action_);
    context_menu_->addAction(share_to_user_action_);
    context_menu_->addAction(share_to_group_action_);
    context_menu_->addSeparator();
    context_menu_->addAction(move_action_);
    context_menu_->addAction(copy_action_);
    context_menu_->addAction(paste_action_);
    context_menu_->addSeparator();
    context_menu_->addAction(lock_action_);
    context_menu_->addAction(rename_action_);
    context_menu_->addAction(remove_action_);
    context_menu_->addSeparator();
    // context_menu_->addAction(update_action_);
    context_menu_->addAction(cancel_download_action_);
    context_menu_->addAction(sync_subdirectory_action_);
    context_menu_->addSeparator();
    context_menu_->addAction(open_local_cache_folder_action_);

    this->addAction(retry_upload_cached_file_action_);
    this->addAction(delete_local_version_action_);
    this->addAction(local_version_saveas_action_);
    this->addAction(saveas_action_);
    this->addAction(share_action_);
    this->addAction(share_seafile_action_);
    this->addAction(move_action_);
    this->addAction(copy_action_);
    this->addAction(paste_action_);
    this->addAction(lock_action_);
    this->addAction(rename_action_);
    this->addAction(remove_action_);
    // this->addAction(update_action_);
    this->addAction(cancel_download_action_);
    this->addAction(sync_subdirectory_action_);
    this->addAction(open_local_cache_folder_action_);

    paste_only_menu_ = new QMenu(this);
    paste_only_menu_->addAction(paste_action_);
}

void FileTableView::contextMenuEvent(QContextMenuEvent *event)
{
    QPoint position = event->pos();
    const QModelIndex proxy_index = indexAt(position);
    position = viewport()->mapToGlobal(position);

    retry_upload_cached_file_action_->setVisible(false);
    delete_local_version_action_->setVisible(false);
    local_version_saveas_action_->setVisible(false);
    open_local_cache_folder_action_->setEnabled(false);

    //
    // paste_action shows only if there are files in the clipboard
    // and is enabled only if it comes from the same account
    //
    paste_action_->setVisible(parent_->hasFilesToBePasted());

    //
    // show paste only menu for no items
    // paste-dedicated menu
    //
    paste_action_->setEnabled(!parent_->current_readonly_ &&
                              parent_->account_to_be_pasted_from_ ==
                                  parent_->account_ &&
                              parent_->hasFilesToBePasted());
    if (!proxy_index.isValid()) {
        if (parent_->hasFilesToBePasted()) {
            paste_only_menu_->exec(position);
        }
        return;
    }

    // printf ("paste action enabled: %s\n", paste_action_->isEnabled() ? "true": "false");

    //
    // map back to the source index from FileTableModel
    //
    const QModelIndex index = proxy_model_->mapToSource(proxy_index);
    const int row = index.row();
    const SeafDirent *dirent = source_model_->direntAt(row);
    // if invalid dirent? no sure why it comes
    if (!dirent)
        return;

    //
    // find if the item is in the selection
    // get selections
    QItemSelectionModel *selections = this->selectionModel();
    QModelIndexList selected = selections->selectedRows();
    int pos;
    for (pos = 0; pos < selected.size(); pos++)
    {
        if (row == proxy_model_->mapToSource(selected[pos]).row())
            break;
    }
    //
    // if the item is in the selection
    // but it is a multi-selection
    //
    // the situation is different from the single-selection
    // supports: download only (and cancel download action perhaps?)
    //
    if (pos != selected.size() && selected.size() != 1) {
        item_.reset(NULL);

        bool has_dir = false;
        for (int i = 0; i != selected.size() ; i++) {
            if (source_model_->direntAt(proxy_model_->mapToSource(selected[i]).row())->isDir()) {
                has_dir = true;
                break;
            }
        }

        saveas_action_->setText(tr("&Save As To..."));
        saveas_action_->setVisible(true);
        saveas_action_->setEnabled(!has_dir);
        lock_action_->setVisible(false);
        rename_action_->setVisible(false);
        share_action_->setVisible(false);
        share_seafile_action_->setVisible(false);
        share_to_user_action_->setVisible(false);
        share_to_group_action_->setVisible(false);
        // update_action_->setVisible(false);
        cancel_download_action_->setVisible(false);
        sync_subdirectory_action_->setVisible(false);

        context_menu_->exec(position);

        return;
    }

    //
    // if the item is not in the selection
    // and it is the single-selection situation
    //
    // it is the most common case
    //

    item_.reset(new SeafDirent(*dirent));

    saveas_action_->setText(tr("&Save As..."));
    saveas_action_->setVisible(true);
    rename_action_->setVisible(true);
    share_action_->setVisible(true);
    share_seafile_action_->setVisible(true);
    // update_action_->setVisible(true);
    cancel_download_action_->setVisible(true);
    cancel_download_action_->setVisible(false);
    if (item_->readonly) {
        move_action_->setEnabled(false);
        rename_action_->setEnabled(false);
        // update_action_->setEnabled(false);
        remove_action_->setEnabled(false);
    } else {
        move_action_->setEnabled(true);
        rename_action_->setEnabled(true);
        // update_action_->setEnabled(true);
        remove_action_->setEnabled(true);
    }

    if (item_->isDir()) {
        lock_action_->setVisible(false);
        // update_action_->setVisible(false);
        saveas_action_->setEnabled(false);
        sync_subdirectory_action_->setVisible(true);
        share_to_user_action_->setVisible(true);
        share_to_group_action_->setVisible(true);
    } else {
        if (item_->locked_by_me) {
            lock_action_->setText(tr("Un&lock"));
            lock_action_->setVisible(true);
            move_action_->setEnabled(true);
            remove_action_->setEnabled(true);
            rename_action_->setEnabled(true);
        } else if (item_->is_locked || item_->readonly) {
            lock_action_->setVisible(false);
            move_action_->setEnabled(false);
            remove_action_->setEnabled(false);
            rename_action_->setEnabled(false);
        } else {
            lock_action_->setText(tr("&Lock"));
            lock_action_->setVisible(true);
            move_action_->setEnabled(true);
            remove_action_->setEnabled(true);
            rename_action_->setEnabled(true);
        }

        // update_action_->setVisible(true);
        saveas_action_->setEnabled(true);
        sync_subdirectory_action_->setVisible(false);
        share_to_user_action_->setVisible(false);
        share_to_group_action_->setVisible(false);

        if (TransferManager::instance()->getDownloadTask(parent_->repo_.id,
            ::pathJoin(parent_->current_path_, dirent->name))) {
            cancel_download_action_->setVisible(true);
            saveas_action_->setVisible(false);
        }
    }

    if (!parent_->account_.isPro() ||
        parent_->repo_.owner != parent_->account_.username) {
        share_to_user_action_->setVisible(false);
        share_to_group_action_->setVisible(false);
    }

    QVariant file_status_v = source_model_->data(index, DirentCacheStatusRole);
    AutoUpdateManager::FileStatus file_status = (AutoUpdateManager::FileStatus)file_status_v.toInt();
    if (file_status == AutoUpdateManager::NOT_SYNCED) {
        retry_upload_cached_file_action_->setVisible(true);
        delete_local_version_action_->setVisible(true);
        local_version_saveas_action_->setVisible(true);
    }

    QString localpath = DataManager::getLocalCacheFilePath(
        parent_->repo_.id, ::pathJoin(parent_->current_path_, dirent->name));
    if (QFileInfo(localpath).exists()) {
        open_local_cache_folder_action_->setEnabled(true);
    }

    context_menu_->exec(position); // synchronously

    //
    // reset it to NULL, when the menu exec is done
    //
    item_.reset(NULL);
}

void FileTableView::onAboutToReset()
{
    item_.reset(NULL);
}

void FileTableView::onItemDoubleClicked(const QModelIndex& index)
{
    const SeafDirent *dirent =
      source_model_->direntAt(proxy_model_->mapToSource(index).row());

    if (dirent == NULL)
        return;

    emit direntClicked(*dirent);
}

void FileTableView::onRetryUploadCachedFile()
{
    const SeafDirent *dirent = getSelectedItemFromSource();
    parent_->onGetDirentReupload(*dirent);
}

void FileTableView::onDeleteLocalVersion()
{
    const SeafDirent *dirent = getSelectedItemFromSource();

    if (dirent == NULL) {
        return;
    }

    emit deleteLocalVersion(*dirent);
}

void FileTableView::onLocalVersionSaveAs()
{
    const SeafDirent *dirent = getSelectedItemFromSource();

    if (dirent == NULL) {
        return;
    }

    emit localVersionSaveAs(*dirent);
}

void FileTableView::onSaveAs()
{
    if (item_ == NULL) {
        const QList<const SeafDirent*> dirents = getSelectedItemsFromSource();
        emit direntSaveAs(dirents);
        return;
    }

    QList<const SeafDirent*> dirents;
    dirents.push_back(item_.data());
    emit direntSaveAs(dirents);
}

void FileTableView::onLock()
{
    const SeafDirent * item = item_.data();
    if (item == NULL)
        item = getSelectedItemFromSource();
    if (!item || item->isDir() || item->readonly)
        return;

    emit direntLock(*item);
}

void FileTableView::onRename()
{
    if (item_ == NULL) {
        const SeafDirent *selected_item = getSelectedItemFromSource();
        if (selected_item && !selected_item->readonly)
            emit direntRename(*selected_item);
        return;
    }

    if (!item_->readonly)
        emit direntRename(*item_);
}

void FileTableView::onRemove()
{
    bool has_readonly = false;
    if (item_ == NULL) {
        const QList<const SeafDirent*> dirents = getSelectedItemsFromSource();
        Q_FOREACH(const SeafDirent* dirent, dirents)
        {
            if (dirent->readonly) {
                has_readonly = true;
                break;
            }
        }
        if (!has_readonly) {
            emit direntRemove(dirents);
            return;
        }
    }
    if (has_readonly || item_->readonly) {
        seafApplet->messageBox(tr("Unable to remove readonly files"), this);
    }

    emit direntRemove(*item_);
}

void FileTableView::onShare()
{
    if (item_ == NULL) {
        const SeafDirent *selected_item = getSelectedItemFromSource();
        if (selected_item && selected_item->isFile())
            emit direntShare(*selected_item);
        return;
    }
    emit direntShare(*item_);
}

void FileTableView::onShareToUser()
{
    onShareToUserOrGroup(false);
}

void FileTableView::onShareToGroup()
{
    onShareToUserOrGroup(true);
}

void FileTableView::onShareToUserOrGroup(bool to_group)
{
    if (item_ == NULL) {
        const SeafDirent *selected_item = getSelectedItemFromSource();
        if (selected_item && selected_item->isDir())
            emit direntShareToUserOrGroup(*selected_item, to_group);
        return;
    }
    emit direntShareToUserOrGroup(*item_, to_group);
}


void FileTableView::onShareSeafile()
{
    if (item_ == NULL) {
        const SeafDirent *selected_item = getSelectedItemFromSource();
        if (selected_item && selected_item->isFile())
            emit direntShareSeafile(*selected_item);
        return;
    }
    emit direntShareSeafile(*item_);
}


void FileTableView::onUpdate()
{
    if (item_ == NULL) {
        const SeafDirent *selected_item = getSelectedItemFromSource();
        if (selected_item && selected_item->isFile() && !selected_item->readonly)
            emit direntUpdate(*selected_item);
        return;
    }
    if (!item_->readonly)
        emit direntUpdate(*item_);
}

void FileTableView::onCopy()
{
    QStringList file_names;

    if (item_ == NULL) {
        const QList<const SeafDirent*> dirents = getSelectedItemsFromSource();
        for (int i = 0; i < dirents.size(); i++) {
            file_names.push_back(dirents[i]->name);
        }
    } else {
        file_names.push_back(item_->name);
    }


    parent_->setFilesToBePasted(true, file_names);
}

void FileTableView::onMove()
{
    QStringList file_names;
    bool has_readonly = false;

    if (item_ == NULL) {
        const QList<const SeafDirent*> dirents = getSelectedItemsFromSource();
        for (int i = 0; i < dirents.size(); i++) {
            // unable to move readonly files
            if (dirents[i]->readonly) {
                has_readonly = true;
                break;
            }
            file_names.push_back(dirents[i]->name);
        }
    } else {
        if (item_->readonly) {
            has_readonly = true;
        } else {
            file_names.push_back(item_->name);
        }
    }

    if (has_readonly) {
        seafApplet->messageBox(tr("Unable to cut readonly files"), this);
        return;
    }

    parent_->setFilesToBePasted(false, file_names);
}

void FileTableView::onCancelDownload()
{
    if (item_ == NULL) {
        const QList<const SeafDirent*> dirents = getSelectedItemsFromSource();
        for (int i = 0; i < dirents.size(); i++) {
            emit cancelDownload(*dirents[i]);
        }
        return;
    }
    emit cancelDownload(*item_);
}

void FileTableView::onSyncSubdirectory()
{
    if (item_ && item_->isDir())
        emit syncSubdirectory(item_->name);
}

void FileTableView::onOpenLocalCacheFolder()
{
    parent_->onOpenLocalCacheFolder();
}

void FileTableView::resizeEvent(QResizeEvent *event)
{
    QTableView::resizeEvent(event);
    if (source_model_)
        source_model_->onResize(event->size());
}

FileTableModel::FileTableModel(QObject *parent)
    : QAbstractTableModel(parent),
     name_column_width_(kFileNameColumnWidth)
{
    task_progress_timer_ = new QTimer(this);
    connect(task_progress_timer_, SIGNAL(timeout()),
            this, SLOT(updateDownloadInfo()));
    connect(task_progress_timer_, SIGNAL(timeout()),
            this, SLOT(updateFileCacheStatus()));
    connect(ThumbnailService::instance(), SIGNAL(thumbnailUpdated(const QPixmap&, const QString&)),
            this, SLOT(updateThumbnail(const QPixmap &, const QString&)));
    task_progress_timer_->start(kRefreshProgressInterval);
}

void FileTableModel::setDirents(const QList<SeafDirent>& dirents)
{
    beginResetModel();
    dirents_ = dirents;
    progresses_.clear();
    file_cache_statuses_.clear();
    endResetModel();

    updateFileCacheStatus();
    updateDownloadInfo();
    task_progress_timer_->start();
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

    if (role == Qt::DecorationRole && column == FILE_COLUMN_NAME) {
        QIcon icon;
        if (dirent.isDir()) {
            icon = dirent.readonly
                       ? QIcon(":/images/files_v2/file_folder_readonly.png")
                       : QIcon(":/images/files_v2/file_folder.png");
        } else if (iconPrefixFromFileName(dirent.name) == "image") {
            FileBrowserDialog *dialog =
                (FileBrowserDialog *)(QObject::parent());
            ThumbnailService *service = ThumbnailService::instance();
            return service->getThumbnail(
                dialog->repo_.id,
                ::pathJoin(dialog->current_path_, dirent.name),
                dirent.id,
                kColumnIconSize * globalDevicePixelRatio());
        } else {
            icon = QIcon(getIconByFileNameV2(dirent.name));
        }
        return icon.pixmap(kColumnIconSize, kColumnIconSize);
    }

    if (role == Qt::SizeHintRole) {
        QSize qsize(kDefaultColumnWidth, kDefaultColumnHeight);
        switch (column) {
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

    if (role == Qt::UserRole && column == FILE_COLUMN_KIND) {
        return dirent.isDir() ? readableNameForFolder() : readableNameForFile(dirent.name);
    }

    if (role == DirentLockStatusRole && column == FILE_COLUMN_NAME) {
        if (dirent.locked_by_me) {
            return (int)LOCKED_BY_ME;
        } else if (dirent.is_locked) {
            return (int)LOCKED_BY_OTHERS;
        } else
            return NOT_LOCKED;
    }

    if (role == DirentLockOwnerRole && column == FILE_COLUMN_NAME) {
        return dirent.lock_owner;
    }

    if (role == Qt::ToolTipRole && column == FILE_COLUMN_NAME && !dirent.lock_owner.isEmpty()) {
        return tr("locked by %1").arg(dirent.getLockOwnerDisplayString());
    }

    if (role == DirentCacheStatusRole) {
        return file_cache_statuses_.contains(dirent.name)
                   ? (int)file_cache_statuses_[dirent.name]
                   : QVariant();
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
        return dirent.size;
    case FILE_COLUMN_MTIME:
        return dirent.mtime;
    case FILE_COLUMN_KIND:
        // workaround with sorting
        return dirent.isDir() ? "" : iconPrefixFromFileName(dirent.name);
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
    if (orientation == Qt::Vertical) {
        return QVariant();
    }

    if (role == Qt::TextAlignmentRole) {
        return Qt::AlignLeft + Qt::AlignVCenter;
    }

    if (role == Qt::DisplayRole) {
        switch (section) {
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

    if (role == Qt::FontRole) {
        QFont font;
        font.setPixelSize(12);
        return font;
    }

    if (role == Qt::ForegroundRole) {
        return QBrush(kFontColor);
    }

    if (role == Qt::SizeHintRole && section == FILE_COLUMN_NAME) {
        if (dirents_.empty()) {
            return QSize(name_column_width_, 0);
        }
    }

    return QVariant();
}

const SeafDirent* FileTableModel::direntAt(int row) const
{
    if (row >= dirents_.size())
        return NULL;

    return &dirents_[row];
}

void FileTableModel::replaceItem(const QString &name, const SeafDirent &dirent)
{
    for (int pos = 0; pos != dirents_.size() ; pos++)
        if (dirents_[pos].name == name) {
            dirents_[pos] = dirent;
            emit dataChanged(index(pos, 0), index(pos , FILE_MAX_COLUMN - 1));
            break;
        }
}

void FileTableModel::insertItem(int pos, const SeafDirent &dirent)
{
    if (pos > dirents_.size())
        return;
    beginInsertRows(QModelIndex(), pos, pos);
    dirents_.insert(pos, dirent);
    endInsertRows();
}

void FileTableModel::removeItemNamed(const QString &name)
{
    for (int pos = 0; pos != dirents_.size() ; pos++)
        if (dirents_[pos].name == name) {
            beginRemoveRows(QModelIndex(), pos, pos);
            dirents_.removeAt(pos);
            endRemoveRows();
            break;
        }
}

void FileTableModel::renameItemNamed(const QString &name, const QString &new_name)
{
    for (int pos = 0; pos != dirents_.size() ; pos++)
        if (dirents_[pos].name == name) {
            dirents_[pos].name = new_name;
            emit dataChanged(index(pos, 0), index(pos , FILE_MAX_COLUMN - 1));
            break;
        }
}

void FileTableModel::onResize(const QSize &size)
{
    name_column_width_ = size.width() - kDefaultColumnSum + kFileNameColumnWidth;
    // name_column_width_ should be always larger than kFileNameColumnWidth
    if (dirents_.empty())
        return;
    emit dataChanged(index(0, FILE_COLUMN_NAME),
                     index(dirents_.size()-1 , FILE_COLUMN_NAME));
}

void FileTableModel::updateDownloadInfo()
{
    FileBrowserDialog *dialog = (FileBrowserDialog *)(QObject::parent());
    QList<FileDownloadTask*> tasks= TransferManager::instance()->getDownloadTasks(
        dialog->repo_.id, dialog->current_path_);

    progresses_.clear();

    Q_FOREACH (FileDownloadTask *task, tasks) {
        QString progress = task->progress().toString();
        progresses_[::getBaseName(task->path())] = progress;
    }

    if (dirents_.empty())
        return;
    emit dataChanged(index(0, FILE_COLUMN_SIZE),
                     index(dirents_.size() - 1 , FILE_COLUMN_SIZE));
}


void FileTableModel::updateFileCacheStatus()
{
    if (dirents_.empty())
        return;

    FileBrowserDialog *dialog = (FileBrowserDialog *)(QObject::parent());
    QHash<QString, AutoUpdateManager::FileStatus> new_statues =
        AutoUpdateManager::instance()->getFileStatusForDirectory(
            dialog->account_.getSignature(),
            dialog->repo_.id,
            dialog->current_path_,
            dirents_);

    if (new_statues == file_cache_statuses_) {
        return;
    }

    for (int pos = 0; pos != dirents_.size() ; pos++) {
        const QString& name = dirents_[pos].name;
        int new_status = new_statues.contains(name) ? (int)new_statues[name] : -1;
        int old_status = file_cache_statuses_.contains(name) ? (int)file_cache_statuses_[name] : -1;
        if (new_status != old_status) {
            emit dataChanged(index(pos, FILE_COLUMN_NAME), index(pos , FILE_COLUMN_NAME));
        }
    }

    file_cache_statuses_ = new_statues;
    // printf ("repainting when file cache status is changed\n");
    // emit dataChanged(index(0, FILE_COLUMN_NAME),
    //                  index(dirents_.size() - 1 , FILE_COLUMN_NAME));
}

void FileTableModel::updateThumbnail(const QPixmap& thumbnail, const QString& path)
{
    const QString name = QFileInfo(path).fileName();
    for (int pos = 0; pos != dirents_.size() ; pos++)
        if (dirents_[pos].name == name) {
            emit dataChanged(index(pos, 0), index(pos, FILE_COLUMN_NAME));
            break;
        }
}

bool FileTableSortFilterProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    bool is_dir_left = source_model_->direntAt(left.row())->isDir();
    bool is_dir_right = source_model_->direntAt(right.row())->isDir();
    if (is_dir_left != is_dir_right) {
        return sortOrder() != Qt::AscendingOrder ? is_dir_right
                                                 : !is_dir_right;
    }
    else if ((left.column() == FILE_COLUMN_NAME) &&
             (right.column() == FILE_COLUMN_NAME)) {
        const QString left_name = source_model_->direntAt(left.row())->name;
        const QString right_name = source_model_->direntAt(right.row())->name;
        return digitalCompare(left_name, right_name) < 0;
    }
    else {
        return QSortFilterProxyModel::lessThan(left, right);
    }
}
