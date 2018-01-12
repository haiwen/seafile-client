#include <QtGlobal>

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
#include <QtWidgets>
#else
#include <QtGui>
#endif

#include "file-browser-dialog.h"
#include "file-browser-search-tab.h"
#include "ui/main-window.h"
#include "utils/utils.h"
#include "utils/file-utils.h"
#include "utils/paint-utils.h"
#include "seafile-applet.h"
#include "api/requests.h"
#include "repo-service.h"

namespace {

enum {
    FILE_COLUMN_NAME = 0,
    FILE_COLUMN_MTIME,
    FILE_COLUMN_SIZE,
    FILE_COLUMN_KIND,
    FILE_MAX_COLUMN
};

int kMarginLeft = 9;
const int kDefaultColumnWidth = 120;
const int kDefaultColumnHeight = 40;
const int kColumnIconSize = 28;
const int kFileNameColumnWidth = 200;
const int kExtraPadding = 30;
const int kDefaultColumnSum = kFileNameColumnWidth + kDefaultColumnWidth * 3 + kExtraPadding;
const int kFileStatusIconSize = 16;
const int kMarginBetweenFileNameAndStatusIcon = 5;

const QColor kSelectedItemBackgroundcColor("#F9E0C7");
const QColor kItemBackgroundColor("white");
const QColor kItemBottomBorderColor("#ECECEC");
const QColor kFileNameFontColor("black");
const QColor kFontColor("#757575");

} // anonymous namespace

FileBrowserSearchItemDelegate::FileBrowserSearchItemDelegate(QObject *parent)
    : QStyledItemDelegate(parent) {

}

void FileBrowserSearchItemDelegate::paint(QPainter *painter,
                                     const QStyleOptionViewItem &option,
                                     const QModelIndex &index) const {
    const FileBrowserSearchModel *model = static_cast<const FileBrowserSearchModel*>(index.model());

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

    }
         break;

    case FILE_COLUMN_SIZE:
        if (!text.isEmpty())
            text = ::readableFileSize(model->data(index, Qt::DisplayRole).value<quint64>());
    case FILE_COLUMN_MTIME:
        if (index.column() == FILE_COLUMN_MTIME)
            text = ::translateCommitTime(model->data(index, Qt::DisplayRole).value<quint64>());
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
    default:
        qWarning() << "invalid item (row)";
        break;
    }
}

FileBrowserSearchView::FileBrowserSearchView(QWidget* parent)
    : QTableView(parent),
      parent_(qobject_cast<FileBrowserDialog*>(parent)),
      search_model_(NULL),
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

    connect(this, SIGNAL(doubleClicked(const QModelIndex&)),
                this, SLOT(onItemDoubleClicked(const QModelIndex&)));

    setupContextMenu();
}

void FileBrowserSearchView::setupContextMenu()
{
    context_menu_ = new QMenu(this);
    connect(parent_, SIGNAL(aboutToClose()),
            context_menu_, SLOT(close()));
    open_parent_dir_action_ = new QAction(tr("&Show in folder"), this);
    open_parent_dir_action_->setIcon(QIcon(":/images/toolbar/file-gray.png"));
    open_parent_dir_action_->setIconVisibleInMenu(true);
    open_parent_dir_action_->setStatusTip(tr("Show in folder"));
    connect(open_parent_dir_action_, SIGNAL(triggered()),
            this, SLOT(openParentDir()));
    context_menu_->addAction(open_parent_dir_action_);
    this->addAction(open_parent_dir_action_);
}

void FileBrowserSearchView::contextMenuEvent(QContextMenuEvent *event)
{
    QPoint position = event->pos();
    const QModelIndex proxy_index = indexAt(position);
    position = viewport()->mapToGlobal(position);
    if (!proxy_index.isValid()) {
        return;
    }

    const QModelIndex index = proxy_model_->mapToSource(proxy_index);
    const int row = index.row();
    const FileSearchResult *result = search_model_->resultAt(row);
    if (!result)
        return;

    QItemSelectionModel *selections = this->selectionModel();
    QModelIndexList selected = selections->selectedRows();
    if (selected.size() == 1) {
        open_parent_dir_action_->setEnabled(true);
        search_item_.reset(new FileSearchResult(*result));
    } else {
        open_parent_dir_action_->setEnabled(false);
        return;
    }

    context_menu_->exec(position); // synchronously
    search_item_.reset(NULL);
}

void FileBrowserSearchView::openParentDir()
{
    parent_->enterPath(::getParentPath(search_item_->fullpath));
    emit clearSearchBar();
}

void FileBrowserSearchView::onItemDoubleClicked(const QModelIndex& index)
{
    const FileSearchResult *result =
            search_model_->resultAt(proxy_model_->mapToSource(index).row());
    if (result->name.isEmpty() || result->fullpath.isEmpty())
        return;

    if (result->fullpath.endsWith("/"))
        emit clearSearchBar();

    RepoService::instance()->openLocalFile(result->repo_id, result->fullpath);
}

void FileBrowserSearchView::setModel(QAbstractItemModel *model)
{
    search_model_ = qobject_cast<FileBrowserSearchModel*>(model);
    if (!search_model_)
        return;
    proxy_model_ = new FileSearchSortFilterProxyModel(search_model_);
    proxy_model_->setSourceModel(search_model_);
    QTableView::setModel(proxy_model_);

//    setColumnHidden(FILE_COLUMN_PROGRESS, true);
    connect(model, SIGNAL(modelAboutToBeReset()), this, SLOT(onAboutToReset()));
    setSortingEnabled(true);

    // set default sort by folder
    sortByColumn(FILE_COLUMN_NAME, Qt::AscendingOrder);
}

void FileBrowserSearchView::onAboutToReset()
{
    search_item_.reset(NULL);
}

void FileBrowserSearchView::resizeEvent(QResizeEvent *event)
{
    QTableView::resizeEvent(event);
    if (search_model_)
        search_model_->onResize(event->size());
}

FileBrowserSearchModel::FileBrowserSearchModel(QObject *parent)
    : QAbstractTableModel(parent),
      name_column_width_(kFileNameColumnWidth)
{

}

void FileBrowserSearchModel::setSearchResult(const std::vector<FileSearchResult>& results)
{
    beginResetModel();
    results_ = results;
    endResetModel();
}

int FileBrowserSearchModel::rowCount(const QModelIndex &parent) const
{
    return results_.size();
}

int FileBrowserSearchModel::columnCount(const QModelIndex &index) const
{
    return FILE_MAX_COLUMN;
}

QVariant FileBrowserSearchModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= (int)results_.size()) {
        return QVariant();
    }
    const int column = index.column();
    const int row = index.row();
    const FileSearchResult& result = results_[row];

    if (role == Qt::DecorationRole && column == FILE_COLUMN_NAME) {
        QIcon icon;

        if (result.fullpath.endsWith("/")) {
            icon = QIcon(":/images/files_v2/file_folder.png");
        } else {
            icon = QIcon(getIconByFileNameV2(result.name));
        }

        return icon.pixmap(kColumnIconSize, kColumnIconSize);
    }

    if (role == Qt::SizeHintRole) {
        QSize qsize(kDefaultColumnWidth, kDefaultColumnHeight);
        switch (column) {
        case FILE_COLUMN_NAME:
            qsize.setWidth(name_column_width_);
            break;
        case FILE_COLUMN_SIZE:
        case FILE_COLUMN_MTIME:
        case FILE_COLUMN_KIND:
        default:
            break;
        }
        return qsize;
    }

    if (role == Qt::UserRole && column == FILE_COLUMN_KIND) {
        return result.fullpath.endsWith("/") ? readableNameForFolder() : readableNameForFile(result.name);
    }

    //DisplayRole
    switch (column) {
    case FILE_COLUMN_NAME:

        return result.name;
    case FILE_COLUMN_SIZE:
        if (result.fullpath.endsWith("/"))
            return "";
        return result.size;
    case FILE_COLUMN_MTIME:
        return result.last_modified;
    case FILE_COLUMN_KIND:
    default:
        return QVariant();
    }
}

QVariant FileBrowserSearchModel::headerData(int section,
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
        if (results_.empty()) {
            return QSize(name_column_width_, 0);
        }
    }
    return QVariant();
}

void FileBrowserSearchModel::onResize(const QSize &size)
{
    name_column_width_ = size.width() - kDefaultColumnSum + kFileNameColumnWidth;
    // name_column_width_ should be always larger than kFileNameColumnWidth
    if (results_.empty())
        return;
    emit dataChanged(index(0, FILE_COLUMN_NAME),
                     index(results_.size()-1 , FILE_COLUMN_NAME));
}

const FileSearchResult* FileBrowserSearchModel::resultAt(int row) const
{
    int nSize = static_cast<int>(results_.size());
    if (row >= nSize)
        return NULL;

    return &results_[row];
}

bool FileSearchSortFilterProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    bool is_dir_left = search_model_->resultAt(left.row())->fullpath.endsWith("/");
    bool is_dir_right = search_model_->resultAt(right.row())->fullpath.endsWith("/");
    if (is_dir_left != is_dir_right) {
        return sortOrder() != Qt::AscendingOrder ? is_dir_right
                                                 : !is_dir_right;
    }
    else if ((left.column() == FILE_COLUMN_NAME) &&
             (right.column() == FILE_COLUMN_NAME)) {
        const QString left_name = search_model_->resultAt(left.row())->name;
        const QString right_name = search_model_->resultAt(right.row())->name;
        return digitalCompare(left_name, right_name) < 0;
    }
    else {
        return QSortFilterProxyModel::lessThan(left, right);
    }
}
