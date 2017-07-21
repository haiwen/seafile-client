#include <QtGlobal>

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
#include <QtWidgets>
#else
#include <QtGui>
#endif

#include "ui/search-tab.h"
#include "ui/search-tab-items.h"
#include "ui/main-window.h"
#include "utils/utils.h"
#include "utils/file-utils.h"
#include "utils/paint-utils.h"
#include "seafile-applet.h"
#include "api/requests.h"
#include "repo-service.h"

namespace {
const int kMarginLeft = 5;
const int kMarginRight = 5;
const int kMarginTop = 5;
const int kMarginBottom = 5;
const int kPadding = 5;

const int kFileIconHeight = 36;
const int kFileIconWidth = 36;
const int kFileNameWidth = 210;
const int kFileNameHeight = 30;

const int kSubtitleHeight = 16;

const int kMarginBetweenFileIconAndName = 10;

const char *kFileNameColor = "#3F3F3F";
const char *kFileNameColorHighlighted = "#544D49";
const char *kSubtitleColor = "#959595";
const char *kSubtitleColorHighlighted = "#9D9B9A";
const int kFileNameFontSize = 14;
const int kSubtitleFontSize = 11;

const char *kFileItemBackgroundColor = "white";
const char *kFileItemBackgroundColorHighlighted = "#F9E0C7";

const char *kItemBottomBorderColor = "#EEE";

const int PLACE_HOLDER_TYPE = 999;

static inline const QListWidgetItem *getItem(const QModelIndex &index)
{
    const SearchResultListModel *model = static_cast<const SearchResultListModel*>(index.model());
    return model->item(index);
}
static inline FileSearchResult getSearchResult(const QModelIndex &index)
{
    const QListWidgetItem *item = getItem(index);
    if (!item)
        return FileSearchResult();
    return item->data(Qt::UserRole).value<FileSearchResult>();
}
} // anonymous namespace

void SearchResultItemDelegate::paint(QPainter *painter,
                                     const QStyleOptionViewItem &option,
                                     const QModelIndex &index) const {
    const SearchResultListModel *model = static_cast<const SearchResultListModel*>(index.model());
    const QListWidgetItem* item = getItem(index);
    if (item && item->type() == PLACE_HOLDER_TYPE) {
        // This is the place holder item for the "load more" button
        return;
    }
    QBrush backBrush;
    bool selected = false;
    FileSearchResult file = getSearchResult(index);

    if (option.state & (QStyle::State_HasFocus | QStyle::State_Selected)) {
        backBrush = QColor(kFileItemBackgroundColorHighlighted);
        selected = true;
    } else {
        backBrush = QColor(kFileItemBackgroundColor);
    }

    //
    // draw item's background
    //
    painter->save();
    painter->fillRect(option.rect, backBrush);
    painter->restore();

    QIcon icon = model->data(index, Qt::DecorationRole).value<QIcon>();
    QPixmap pixmap(icon.pixmap(QSize(kFileIconWidth, kFileIconHeight)));
    //
    // paint file icon
    //
    QPoint file_icon_pos(kMarginLeft + kPadding, kMarginTop + kPadding);
    file_icon_pos += option.rect.topLeft();
    painter->save();
    painter->drawPixmap(file_icon_pos, pixmap);
    painter->restore();

    // Calculate the file column by the delta of mainwindow's width
    QString title = file.name;

    const int file_name_width = kFileNameWidth
      + seafApplet->mainWindow()->width() - seafApplet->mainWindow()->minimumWidth();
    painter->save();
    QPoint file_name_pos = file_icon_pos + QPoint(kFileIconWidth + kMarginBetweenFileIconAndName, -kPadding);
    QRect file_name_rect(file_name_pos, QSize(file_name_width, kFileNameHeight));
    painter->setPen(QColor(selected ? kFileNameColorHighlighted : kFileNameColor));
    painter->setFont(changeFontSize(painter->font(), kFileNameFontSize));

    painter->drawText(file_name_rect,
                      Qt::AlignLeft | Qt::AlignTop,
                      fitTextToWidth(title, option.font, file_name_width),
                      &file_name_rect);
    painter->restore();

    //
    // Paint repo_name
    //
    int count_of_splash = file.fullpath.endsWith("/") ? 2 : 1;
    QString subtitle = file.fullpath.mid(1, file.fullpath.size() - count_of_splash - file.name.size());
    if (!subtitle.isEmpty())
        subtitle = file.repo_name + "/" + subtitle.left(subtitle.size() - 1);
    else
        subtitle = file.repo_name;

    painter->save();
    QPoint file_desc_pos = file_name_rect.bottomLeft() + QPoint(0, kPadding / 2);
    QRect file_desc_rect(file_desc_pos, QSize(file_name_width, kSubtitleHeight));
    painter->setFont(changeFontSize(painter->font(), kSubtitleFontSize));
    painter->setPen(QColor(selected ? kSubtitleColorHighlighted : kSubtitleColor));
    painter->drawText(file_desc_rect,
                      Qt::AlignLeft | Qt::AlignTop,
                      fitTextToWidth(subtitle, option.font, file_name_width),
                      &file_desc_rect);
    painter->restore();

    //
    // Paint file description
    //
    QString size, mtime;

    size = readableFileSize(file.size);
    mtime = translateCommitTime(file.last_modified);

    QString extra_title = size + "  " + mtime;

    painter->save();
    QPoint file_extra_pos = file_desc_rect.bottomLeft() + QPoint(0, kPadding / 2 + 2);
    QRect file_extra_rect(file_extra_pos, QSize(file_name_width, kSubtitleHeight));
    painter->setFont(changeFontSize(painter->font(), kSubtitleFontSize));
    painter->setPen(QColor(selected ? kSubtitleColorHighlighted : kSubtitleColor));
    painter->drawText(file_extra_rect,
                      Qt::AlignLeft | Qt::AlignTop,
                      fitTextToWidth(extra_title, option.font, file_name_width),
                      &file_extra_rect);
    painter->restore();

    //
    // Draw the bottom border lines
    //
    painter->save();
    painter->setPen(QPen(QColor(kItemBottomBorderColor), 1, Qt::SolidLine));
    painter->drawLine(option.rect.bottomLeft(), option.rect.bottomRight());
    painter->restore();
}

QSize SearchResultItemDelegate::sizeHint(const QStyleOptionViewItem &option,
                                         const QModelIndex &index) const {

    const QListWidgetItem* item = getItem(index);
    if (!item)
        return QStyledItemDelegate::sizeHint(option, index);

    return QSize(kMarginLeft + kFileIconWidth + kMarginBetweenFileIconAndName + kFileNameWidth + kMarginRight + kPadding * 2,
                 kFileIconHeight + kPadding * 3 + kMarginTop + kMarginBottom);
}

SearchResultListView::SearchResultListView(QWidget* parent) : QListView(parent)
{
#if defined(Q_OS_MAC)
    setAttribute(Qt::WA_MacShowFocusRect, 0);
#endif
    createActions();
}

void SearchResultListView::createActions()
{
    open_parent_dir_action_ = new QAction(tr("&Show in folder"), this);
    open_parent_dir_action_->setIcon(QIcon(":/images/toolbar/file-gray.png"));
    open_parent_dir_action_->setIconVisibleInMenu(true);
    open_parent_dir_action_->setStatusTip(tr("Show in folder"));
    connect(open_parent_dir_action_,
            SIGNAL(triggered()),
            this,
            SLOT(openParentDir()));
}

void SearchResultListView::contextMenuEvent(QContextMenuEvent* event)
{
    QPoint pos = event->pos();
    QModelIndex index = indexAt(pos);
    if (!index.isValid()) {
        return;
    }

    const QListWidgetItem* item = getItem(index);
    if (!item) {
        return;
    }

    updateActions();
    QMenu* menu = prepareContextMenu();
    pos = viewport()->mapToGlobal(pos);
    menu->exec(pos);
}

QMenu* SearchResultListView::prepareContextMenu()
{
    QMenu* menu = new QMenu(this);
    menu->addAction(open_parent_dir_action_);
    return menu;
}

void SearchResultListView::updateActions()
{
    QModelIndexList indexes = selectionModel()->selection().indexes();
    if (indexes.size() != 0) {
        FileSearchResult file = getSearchResult(indexes.at(0));
        open_parent_dir_action_->setData(QVariant::fromValue(file));
        open_parent_dir_action_->setEnabled(true);
    } else {
        open_parent_dir_action_->setEnabled(false);
    }
}

void SearchResultListView::openParentDir()
{
    FileSearchResult result =
        qvariant_cast<FileSearchResult>(open_parent_dir_action_->data());
    RepoService::instance()->openFolder(result.repo_id,
                                        ::getParentPath(result.fullpath));
}

SearchResultListModel::SearchResultListModel() : QAbstractListModel()
{
    has_more_ = false;
}

void SearchResultListModel::addItem(QListWidgetItem *item)
{
    items_.push_back(item);
    emit dataChanged(index(items_.size() - 1), index(items_.size() - 1));
}

const QModelIndex SearchResultListModel::updateSearchResults(
    const std::vector<QListWidgetItem *> &items,
    bool is_loading_more,
    bool has_more)
{
    int first_new_item = 0;

    beginResetModel();
    if (!is_loading_more) {
        first_new_item = 0;
        clear();
    } else {
        if (items_.size() > 0 && items_[items_.size() - 1]->type() == PLACE_HOLDER_TYPE) {
            QListWidgetItem *old_place_holder = items_[items_.size() - 1];
            items_.pop_back();
            first_new_item = items_.size();

            delete old_place_holder;
        }
    }

    items_.insert(items_.end(), items.begin(), items.end());

    // place holder for the "load more" button
    QListWidgetItem *load_more_place_holder = new QListWidgetItem(nullptr, PLACE_HOLDER_TYPE);
    items_.push_back(load_more_place_holder);

    load_more_index_ = QModelIndex();
    if (has_more) {
        load_more_index_ = index(items_.size() - 1);
    }

    endResetModel();

    if (first_new_item) {
        return index(first_new_item);
    }
    return QModelIndex();
}

int SearchResultListModel::rowCount(const QModelIndex &index) const
{
    return items_.size();
}
