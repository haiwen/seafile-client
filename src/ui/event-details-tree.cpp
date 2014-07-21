#include <QHeaderView>
#include <QFileInfo>
#include <QIcon>

#include "utils/file-utils.h"
#include "repo-service.h"

#include "event-details-tree.h"

EventDetailsFileItem::EventDetailsFileItem(const QString& path, EType etype)
    : path_(path),
      etype_(etype)
{
    // setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    setEditable(false);
}

QString EventDetailsFileItem::name() const
{
    return QFileInfo(path_).fileName();
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

EventDetailsCategoryItem::EventDetailsCategoryItem(const QString& text)
    : QStandardItem(text)
{
    setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    setEditable(false);
}


EventDetailsTreeView::EventDetailsTreeView(const SeafEvent& event, QWidget *parent)
    : QTreeView(parent),
      event_(event)
{
    header()->hide();
    setExpandsOnDoubleClick(false);
#if defined(Q_OS_MAC)
    this->setAttribute(Qt::WA_MacShowFocusRect, 0);
#endif

    setEditTriggers(QAbstractItemView::NoEditTriggers);

    connect(this, SIGNAL(doubleClicked(const QModelIndex&)),
            this, SLOT(onItemDoubleClicked(const QModelIndex&)));
}

void EventDetailsTreeView::onItemDoubleClicked(const QModelIndex& index)
{
    QStandardItem *qitem = getFileItem(index);
    if (!qitem) {
        return;
    }
    if (qitem->type() == EVENT_DETAILS_FILE_ITEM_TYPE) {
        EventDetailsFileItem *item = (EventDetailsFileItem *)qitem;
        if (item->isFileOpenable()) {
            RepoService::instance()->openLocalFile(event_.repo_id, item->path());
        }
    }
}

QStandardItem* EventDetailsTreeView::getFileItem(const QModelIndex& index)
{
    if (!index.isValid()) {
        return NULL;
    }
    const EventDetailsTreeModel *model = (const EventDetailsTreeModel*)index.model();
    QStandardItem *item = model->itemFromIndex(index);
    if (item->type() != EVENT_DETAILS_FILE_ITEM_TYPE) {
        return NULL;
    }
    return item;
}

EventDetailsTreeModel::EventDetailsTreeModel(const SeafEvent& event, QObject *parent)
    : QStandardItemModel(parent),
      event_(event)
{
}

void EventDetailsTreeModel::setCommitDetails(const CommitDetails& details)
{
    clear();

    details_ = details;

    processEventCategory(details.added_files, tr("Added files"),  EventDetailsFileItem::FILE_ADDED);
    processEventCategory(details.deleted_files, tr("Deleted files"),  EventDetailsFileItem::FILE_DELETED);
    processEventCategory(details.modified_files, tr("Modified files"),  EventDetailsFileItem::FILE_MODIFIED);

    processEventCategory(details.added_dirs, tr("Added folders"),  EventDetailsFileItem::DIR_ADDED);
    processEventCategory(details.deleted_dirs, tr("Deleted folders"),  EventDetailsFileItem::DIR_DELETED);

    // renamed files is a list of (before rename, after rename) pair
    std::vector<QString> renamed_files;
    for (int i = 0, n = details.renamed_files.size(); i < n; i++) {
        renamed_files.push_back(details.renamed_files[i].second);
    }
    processEventCategory(renamed_files, tr("Renamed files"),  EventDetailsFileItem::FILE_RENAMED);
}

void EventDetailsTreeModel::processEventCategory(const std::vector<QString>& files,
                                                 const QString& desc,
                                                 EventDetailsFileItem::EType etype)
{
    if (files.empty()) {
        return;
    }

    EventDetailsCategoryItem *category = new EventDetailsCategoryItem(desc);
    appendRow(category);

    for (int i = 0, n = files.size(); i < n; i++) {
        EventDetailsFileItem *item = new EventDetailsFileItem(files[i], etype);
        category->appendRow(item);
    }
}
