#include <QtGlobal>
#include <QFileInfo>

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
#include <QtWidgets>
#else
#include <QtGui>
#endif
#include "QtAwesome.h"

#include "utils/file-utils.h"
#include "utils/paint-utils.h"
#include "repo-service.h"

#include "event-details-tree.h"
namespace {
const int kMarginLeft = 10;
const int kMarginRight = 10;
const int kPadding = 5;
const int kFileIconHeight = 12;
const int kFileItemHeight = 30;
const int kFileItemWidth = 300;

const int kFileNameWidth = 260;

const int kFileNameFontSize = 14;

const char *kFileItemBackgroundColor = "white";
const char *kFileItemBackgroundColorHighlighted = "#F9E0C7";
} // anonymous namespace

/**
 *
 *  status-icon file-path         status-text
 */

void EventDetailsFileItemDelegate::paint(QPainter *painter,
                                         const QStyleOptionViewItem &option,
                                         const QModelIndex &index) const {
    QBrush backBrush;
    bool selected = false;

    if (option.state & (QStyle::State_HasFocus | QStyle::State_Selected)) {
        backBrush = QColor(kFileItemBackgroundColorHighlighted);
        selected = true;

    } else {
        backBrush = QColor(kFileItemBackgroundColor);
    }

    painter->save();
    painter->fillRect(option.rect, backBrush);
    painter->restore();

    if (!index.isValid())
        return;

    const EventDetailsListModel* model = static_cast<const EventDetailsListModel*>(index.model());
    EventDetailsFileItem *item = static_cast<EventDetailsFileItem*>(model->item(index.row()));

    const int height = option.rect.height();
    // get the device pixel radio from current painter device
    int scale_factor = 1;
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    scale_factor = globalDevicePixelRatio();
#endif // QT5
    QPixmap icon(item->etype_icon().pixmap(QSize(kFileIconHeight, kFileIconHeight) * scale_factor).scaledToHeight(kFileIconHeight * scale_factor));
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    icon.setDevicePixelRatio(scale_factor);
#endif // QT5
    //
    // draw status-icon
    //

    QPoint status_icon_pos = option.rect.topLeft() + QPoint(kMarginLeft, height / 2 - kFileIconHeight / 2);
    painter->save();
    painter->drawPixmap(status_icon_pos, icon);
    painter->restore();

    //
    // draw file-path
    //
    const int file_path_width = static_cast<QWidget*>(parent())->width() - kFileItemWidth + kFileNameWidth;
    QString path = item->path();
    if (item->etype() == EventDetailsFileItem::FILE_RENAMED)
        path = item->original_path() + " -> " + item->path();
    painter->save();
    QPoint file_name_pos = option.rect.topLeft() + QPoint(kMarginLeft + kFileIconHeight + kPadding, 0);
    QRect file_name_rect(file_name_pos, QSize(file_path_width, height));
    painter->setPen(QColor(item->etype_color()));
    painter->setFont(changeFontSize(painter->font(), kFileNameFontSize));

    painter->drawText(file_name_rect,
                      Qt::AlignLeft | Qt::AlignVCenter,
                      fitTextToWidth(path, option.font, file_path_width),
                      &file_name_rect);
    painter->restore();
}

QSize EventDetailsFileItemDelegate::sizeHint(const QStyleOptionViewItem &option,
                                             const QModelIndex &index) const {
    if (!index.isValid())
        return QStyledItemDelegate::sizeHint(option, index);

    return QSize(kFileItemWidth, kFileItemHeight);
}

EventDetailsFileItem::EventDetailsFileItem(const QString& path, EType etype, const QString& original_path)
    : path_(path),
      original_path_(original_path),
      etype_(etype)
{
    // setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    setEditable(false);
}

QString EventDetailsFileItem::name() const
{
    return path_;
}

QString EventDetailsFileItem::etype_desc() const
{
    switch(etype_) {
      case FILE_ADDED:
        return QObject::tr("Added");
      case FILE_DELETED:
        return QObject::tr("Deleted");
      case FILE_MODIFIED:
        return QObject::tr("Modified");
      case FILE_RENAMED:
        return QObject::tr("Renamed");
      case DIR_ADDED:
        return QObject::tr("Added");
      case DIR_DELETED:
        return QObject::tr("Deleted");
    };
}

QIcon EventDetailsFileItem::etype_icon() const
{
    switch(etype_) {
      case FILE_ADDED:
        return awesome->icon(icon_plus, QColor("#6CC644"));
      case FILE_DELETED:
        return awesome->icon(icon_minus, QColor("#BD2C00"));
      case FILE_MODIFIED:
        return awesome->icon(icon_pencil, QColor("#D0B44C"));
      case FILE_RENAMED:
        return awesome->icon(icon_arrow_right, QColor("#677A85"));
      case DIR_ADDED:
        return awesome->icon(icon_plus, QColor("#6CC644"));
      case DIR_DELETED:
        return awesome->icon(icon_minus, QColor("#BD2C00"));
    };
}

const char* EventDetailsFileItem::etype_color() const
{
    switch(etype_) {
      case FILE_ADDED:
        return "#6CC644";
      case FILE_DELETED:
        return "#BD2C00";
      case FILE_MODIFIED:
        return "#D0B44C";
      case FILE_RENAMED:
        return "#677A85";
      case DIR_ADDED:
        return "#6CC644";
      case DIR_DELETED:
        return "#BD2C00";
    };
}

bool EventDetailsFileItem::isFileOpenable() const
{
    return etype_ == FILE_ADDED ||
        etype_ == FILE_MODIFIED ||
        etype_ == FILE_RENAMED ||
        etype_ == DIR_ADDED;
}

QVariant EventDetailsFileItem::data(int role) const
{
    if (role == Qt::DecorationRole) {
        if (etype_ == DIR_ADDED || etype_ == DIR_DELETED) {
            return QIcon(":/images/folder.png");
        }
        return QIcon(::getIconByFileName(name()));
    } else if (role == Qt::DisplayRole) {
        return name();
    } else if (role == Qt::ToolTipRole) {
        return path_;
    } else {
        return QVariant();
    }
}


EventDetailsListView::EventDetailsListView(const SeafEvent& event, QWidget *parent)
    : QListView(parent),
      event_(event)
{
#if defined(Q_OS_MAC)
    this->setAttribute(Qt::WA_MacShowFocusRect, 0);
#endif

    setItemDelegate(new EventDetailsFileItemDelegate(this));

    setEditTriggers(QAbstractItemView::NoEditTriggers);

    connect(this, SIGNAL(doubleClicked(const QModelIndex&)),
            this, SLOT(onItemDoubleClicked(const QModelIndex&)));

    context_menu_ = new QMenu(this);
    open_action_ = new QAction(tr("&Open"), this);
    connect(open_action_, SIGNAL(triggered()),
            this, SLOT(onOpen()));

    open_in_file_browser_action_ = new QAction(tr("Open &parent folder"), this);
    connect(open_in_file_browser_action_, SIGNAL(triggered()),
            this, SLOT(onOpenInFileBrowser()));

    context_menu_->addAction(open_action_);
    context_menu_->addAction(open_in_file_browser_action_);
}

void EventDetailsListView::onItemDoubleClicked(const QModelIndex& index)
{
    QStandardItem *qitem = getFileItem(index);
    if (!qitem) {
        return;
    }
    EventDetailsFileItem *item = (EventDetailsFileItem *)qitem;
    if (item->isFileOpenable()) {
        RepoService::instance()->openLocalFile(event_.repo_id, item->path());
    }
}

void EventDetailsListView::onOpen()
{
    if (!item_in_action_)
        return;
    RepoService::instance()->openLocalFile(event_.repo_id, item_in_action_->path());
}

void EventDetailsListView::onOpenInFileBrowser()
{
    if (!item_in_action_)
        return;
    RepoService::instance()->openFolder(event_.repo_id, QFileInfo(item_in_action_->path()).dir().path());
}

void EventDetailsListView::contextMenuEvent(QContextMenuEvent *event)
{
    QPoint position = event->pos();
    const QModelIndex index = indexAt(position);
    position = viewport()->mapToGlobal(position);

    if (!index.isValid()) {
        return;
    }

    const EventDetailsListModel* list_model = static_cast<EventDetailsListModel*>(model());
    item_in_action_ = static_cast<EventDetailsFileItem*>(list_model->item(index.row()));
    open_action_->setEnabled(item_in_action_->isFileOpenable());

    context_menu_->exec(position); // synchronously
}

QStandardItem* EventDetailsListView::getFileItem(const QModelIndex& index)
{
    if (!index.isValid()) {
        return NULL;
    }
    const EventDetailsListModel *model = (const EventDetailsListModel*)index.model();
    QStandardItem *item = model->itemFromIndex(index);
    return item;
}

EventDetailsListModel::EventDetailsListModel(const SeafEvent& event, QObject *parent)
    : QStandardItemModel(parent),
      event_(event)
{
}

void EventDetailsListModel::setCommitDetails(const CommitDetails& details)
{
    clear();

    details_ = details;

    processEventCategory(details.added_files, tr("Added files"),  EventDetailsFileItem::FILE_ADDED);
    processEventCategory(details.deleted_files, tr("Deleted files"),  EventDetailsFileItem::FILE_DELETED);
    processEventCategory(details.modified_files, tr("Modified files"),  EventDetailsFileItem::FILE_MODIFIED);

    processEventCategory(details.added_dirs, tr("Added folders"),  EventDetailsFileItem::DIR_ADDED);
    processEventCategory(details.deleted_dirs, tr("Deleted folders"),  EventDetailsFileItem::DIR_DELETED);

    // renamed files is a list of (before rename, after rename) pair
    for (int i = 0, n = details.renamed_files.size(); i < n; i++) {
        EventDetailsFileItem *item = new EventDetailsFileItem(details.renamed_files[i].second, EventDetailsFileItem::FILE_RENAMED, details.renamed_files[i].first);
        appendRow(item);
    }
}

void EventDetailsListModel::processEventCategory(const std::vector<QString>& files,
                                                 const QString& desc,
                                                 EventDetailsFileItem::EType etype)
{
    if (files.empty()) {
        return;
    }

    for (int i = 0, n = files.size(); i < n; i++) {
        EventDetailsFileItem *item = new EventDetailsFileItem(files[i], etype);
        appendRow(item);
    }
}
