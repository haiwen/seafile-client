#include <QtGlobal>

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
#include <QtWidgets>
#include <QUrlQuery>
#else
#include <QtGui>
#endif
#include <QHeaderView>
#include <QDesktopServices>
#include <QEvent>
#include <QShowEvent>
#include <QHideEvent>

#include "utils/utils.h"
#include "seafile-applet.h"
#include "account-mgr.h"
#include "repo-service.h"
#include "starred-file-item.h"
#include "starred-files-list-model.h"

#include "starred-files-list-view.h"


StarredFilesListView::StarredFilesListView(QWidget *parent)
    : QListView(parent)
{
#if defined(Q_OS_MAC)
    setAttribute(Qt::WA_MacShowFocusRect, 0);
#endif

    createActions();

    connect(this, SIGNAL(doubleClicked(const QModelIndex&)),
            this, SLOT(onItemDoubleClicked(const QModelIndex&)));
}

void StarredFilesListView::createActions()
{
    open_file_action_ = new QAction(tr("&Open"), this);
    open_file_action_->setIcon(QIcon(":/images/folder-open-gray.png"));
    open_file_action_->setIconVisibleInMenu(true);
    open_file_action_->setStatusTip(tr("Open this file"));
    connect(open_file_action_, SIGNAL(triggered()), this, SLOT(openLocalFile()));

    view_file_on_web_action_ = new QAction(tr("view on &Web"), this);
    view_file_on_web_action_->setIcon(QIcon(":/images/cloud-gray.png"));
    view_file_on_web_action_->setIconVisibleInMenu(true);
    view_file_on_web_action_->setStatusTip(tr("view this file on website"));
    connect(view_file_on_web_action_, SIGNAL(triggered()), this, SLOT(viewFileOnWeb()));
}

void StarredFilesListView::openLocalFile()
{
    StarredFile file = qvariant_cast<StarredFile>(view_file_on_web_action_->data());

    openLocalFile(file);
}

void StarredFilesListView::viewFileOnWeb()
{
    StarredFile file = qvariant_cast<StarredFile>(view_file_on_web_action_->data());

    const Account& account = seafApplet->accountManager()->currentAccount();
    if (account.isValid()) {
        QUrl url = account.getAbsoluteUrl("repo/" + file.repo_id + "/files/");
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
        QUrlQuery urlQuery(url);
        urlQuery.addQueryItem("p", file.path);
        url.setQuery(urlQuery);
#else
        url.addQueryItem("p", file.path);
#endif

        QDesktopServices::openUrl(url);
    }
}

void StarredFilesListView::updateActions()
{
    StarredFileItem *item = NULL;
    QItemSelection selected = selectionModel()->selection();
    QModelIndexList indexes = selected.indexes();
    if (indexes.size() != 0) {
        const QModelIndex& index = indexes.at(0);
        QStandardItem *it = ((StarredFilesListModel *)model())->itemFromIndex(index);
        item = (StarredFileItem *)it;
    }

    if (!item) {
        // No item is selected
        open_file_action_->setEnabled(false);
        view_file_on_web_action_->setEnabled(false);
        return;
    }

    const StarredFile& file = item->file();

    open_file_action_->setData(QVariant::fromValue(file));
    view_file_on_web_action_->setData(QVariant::fromValue(file));
}

void StarredFilesListView::contextMenuEvent(QContextMenuEvent *event)
{
    QPoint pos = event->pos();
    QModelIndex index = indexAt(pos);
    if (!index.isValid()) {
        return;
    }

    QStandardItem *item = getFileItem(index);
    if (!item) {
        return;
    }
    updateActions();
    QMenu *menu = prepareContextMenu((StarredFileItem *)item);
    pos = viewport()->mapToGlobal(pos);
    menu->exec(pos);
}

QStandardItem*
StarredFilesListView::getFileItem(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return NULL;
    }
    const StarredFilesListModel *model = (const StarredFilesListModel*)index.model();
    QStandardItem *item = model->itemFromIndex(index);
    return item;
}

QMenu* StarredFilesListView::prepareContextMenu(const StarredFileItem *item)
{
    QMenu *menu = new QMenu(this);

    menu->addAction(open_file_action_);
    menu->addAction(view_file_on_web_action_);

    return menu;
}

bool StarredFilesListView::viewportEvent(QEvent *event)
{
    if (event->type() != QEvent::ToolTip && event->type() != QEvent::WhatsThis) {
        return QListView::viewportEvent(event);
    }

    QPoint global_pos = QCursor::pos();
    QPoint viewport_pos = viewport()->mapFromGlobal(global_pos);
    QModelIndex index = indexAt(viewport_pos);
    if (!index.isValid()) {
        return true;
    }

    QStandardItem *qitem = getFileItem(index);
    if (!qitem) {
        return true;
    }

    StarredFileItem *item = (StarredFileItem *)qitem;

    QRect item_rect = visualRect(index);

    QString text = "<p style='white-space:pre'>";
    text += item->file().name();
    text += "</p>";

    QToolTip::showText(QCursor::pos(), text, viewport(), item_rect);

    return true;
}

void StarredFilesListView::onItemDoubleClicked(const QModelIndex& index)
{
    QStandardItem *item = getFileItem(index);
    if (!item) {
        return;
    }

    const StarredFile& file = ((StarredFileItem *)item)->file();

    openLocalFile(file);
}

void StarredFilesListView::openLocalFile(const StarredFile& file)
{
    RepoService::instance()->openLocalFile(file.repo_id, file.path.mid(1), this);
}
