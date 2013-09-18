#include <QtGui>
#include <QHeaderView>
#include <QDesktopServices>
#include <QEvent>
#include <QShowEvent>
#include <QHideEvent>

#include "QtAwesome.h"
#include "utils/utils.h"
#include "seafile-applet.h"
#include "rpc/rpc-client.h"
#include "rpc/local-repo.h"
#include "download-repo-dialog.h"
#include "clone-tasks-dialog.h"
#include "cloud-view.h"
#include "repo-item.h"
#include "repo-item-delegate.h"
#include "repo-tree-model.h"
#include "repo-tree-view.h"
#include "repo-detail-dialog.h"

RepoTreeView::RepoTreeView(CloudView *cloud_view, QWidget *parent)
    : QTreeView(parent),
      cloud_view_(cloud_view)
{
    header()->hide();
    createActions();

    // We draw the indicator ourselves
    setIndentation(0);
    // We handle the click oursevles
    setExpandsOnDoubleClick(false);

    connect(this, SIGNAL(clicked(const QModelIndex&)),
            this, SLOT(onItemClicked(const QModelIndex&)));
}

void RepoTreeView::contextMenuEvent(QContextMenuEvent *event)
{
    QPoint pos = event->pos();
    QModelIndex index = indexAt(pos);
    if (!index.isValid()) {
        // Not clicked at a repo item
        return;
    }

    QStandardItem *item = getRepoItem(index);
    if (!item || item->type() != REPO_ITEM_TYPE) {
        return;
    }

    QMenu *menu = prepareContextMenu((RepoItem *)item);
    pos = viewport()->mapToGlobal(pos);
    menu->exec(pos);
}

QMenu* RepoTreeView::prepareContextMenu(const RepoItem *item)
{
    QMenu *menu = new QMenu(this);
    menu->addAction(show_detail_action_);
    if (item->localRepo().isValid()) {
        menu->addAction(open_local_folder_action_);
    } else {
        menu->addAction(download_action_);
    }

    menu->addAction(view_on_web_action_);

    if (item->localRepo().isValid()) {
        menu->addAction(toggle_auto_sync_action_);
    }

    return menu;
}

void RepoTreeView::updateRepoActions()
{
    RepoItem *item = NULL;
    QItemSelection selected = selectionModel()->selection();
    QModelIndexList indexes = selected.indexes();
    if (indexes.size() != 0) {
        const QModelIndex& index = indexes.at(0);
        QStandardItem *it = ((RepoTreeModel *)model())->itemFromIndex(index);
        if (it && it->type() == REPO_ITEM_TYPE) {
            item = (RepoItem *)it;
        }
    }

    if (!item) {
        // No repo item is selected
        download_action_->setEnabled(false);
        open_local_folder_action_->setEnabled(false);
        toggle_auto_sync_action_->setEnabled(false);
        view_on_web_action_->setEnabled(false);
        show_detail_action_->setEnabled(false);
        return;
    }

    if (item->localRepo().isValid()) {
        const LocalRepo& local_repo = item->localRepo();
        LocalRepo r;
        if (seafApplet->rpcClient()->getLocalRepo(local_repo.id, &r) >= 0) {
            item->setLocalRepo(r);
        }

        download_action_->setEnabled(false);
        open_local_folder_action_->setData(QVariant::fromValue(local_repo));
        open_local_folder_action_->setEnabled(true);

        toggle_auto_sync_action_->setData(QVariant::fromValue(local_repo));
        toggle_auto_sync_action_->setEnabled(true);
        if (local_repo.auto_sync) {
            toggle_auto_sync_action_->setText(tr("Disable auto sync"));
            toggle_auto_sync_action_->setToolTip(tr("Disable auto sync"));
        } else {
            toggle_auto_sync_action_->setText(tr("Enable auto sync"));
            toggle_auto_sync_action_->setToolTip(tr("Enable auto sync"));
        }

    } else {
        download_action_->setEnabled(true);
        download_action_->setData(QVariant::fromValue(item->repo()));
        open_local_folder_action_->setEnabled(false);
        toggle_auto_sync_action_->setEnabled(false);
    }

    view_on_web_action_->setEnabled(true);
    view_on_web_action_->setData(item->repo().id);
    show_detail_action_->setEnabled(true);
    show_detail_action_->setData(QVariant::fromValue(item->repo()));
}

QStandardItem* RepoTreeView::getRepoItem(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return NULL;
    }
    const RepoTreeModel *model = (const RepoTreeModel*)index.model();
    QStandardItem *item = model->itemFromIndex(index);
    if (item->type() != REPO_ITEM_TYPE &&
        item->type() != REPO_CATEGORY_TYPE) {
        return NULL;
    }
    return item;
}

void RepoTreeView::createActions()
{
    show_detail_action_ = new QAction(tr("&Show details"), this);
    show_detail_action_->setIcon(awesome->icon(icon_info_sign));
    show_detail_action_->setStatusTip(tr("Download this library"));
    show_detail_action_->setIconVisibleInMenu(true);
    connect(show_detail_action_, SIGNAL(triggered()), this, SLOT(showRepoDetail()));

    download_action_ = new QAction(tr("&Download this library"), this);
    download_action_->setIcon(awesome->icon(icon_download));
    download_action_->setStatusTip(tr("Download this library"));
    download_action_->setIconVisibleInMenu(true);
    connect(download_action_, SIGNAL(triggered()), this, SLOT(downloadRepo()));

    open_local_folder_action_ = new QAction(tr("&Open folder"), this);
    open_local_folder_action_->setIcon(awesome->icon(icon_folder_open_alt));
    open_local_folder_action_->setStatusTip(tr("open local folder"));
    open_local_folder_action_->setIconVisibleInMenu(true);
    connect(open_local_folder_action_, SIGNAL(triggered()), this, SLOT(openLocalFolder()));

    toggle_auto_sync_action_ = new QAction(tr("Enable auto sync"), this);
    toggle_auto_sync_action_->setStatusTip(tr("Enable auto sync"));
    connect(toggle_auto_sync_action_, SIGNAL(triggered()), this, SLOT(toggleRepoAutoSync()));

    view_on_web_action_ = new QAction(tr("&View on website"), this);
    view_on_web_action_->setIcon(awesome->icon(icon_hand_right));
    view_on_web_action_->setStatusTip(tr("view this library on seahub"));
    view_on_web_action_->setIconVisibleInMenu(true);

    connect(view_on_web_action_, SIGNAL(triggered()), this, SLOT(viewRepoOnWeb()));
}

void RepoTreeView::downloadRepo()
{
    ServerRepo repo = qvariant_cast<ServerRepo>(download_action_->data());
    DownloadRepoDialog dialog(cloud_view_->currentAccount(), repo, this);
    if (dialog.exec() == QDialog::Accepted) {
        CloneTasksDialog tasks_dialog(this);
        tasks_dialog.exec();
    }
}

void RepoTreeView::showRepoDetail()
{
    ServerRepo repo = qvariant_cast<ServerRepo>(show_detail_action_->data());
    RepoDetailDialog dialog(repo, this);
    dialog.exec();
}

void RepoTreeView::openLocalFolder()
{
    LocalRepo repo = qvariant_cast<LocalRepo>(open_local_folder_action_->data());
    QDesktopServices::openUrl(QUrl::fromLocalFile(repo.worktree));
}

void RepoTreeView::toggleRepoAutoSync()
{
    LocalRepo repo = qvariant_cast<LocalRepo>(open_local_folder_action_->data());

    seafApplet->rpcClient()->setRepoAutoSync(repo.id, !repo.auto_sync);

    updateRepoActions();
}

void RepoTreeView::onItemClicked(const QModelIndex& index)
{
    QStandardItem *item = getRepoItem(index);
    if (!item) {
        return;
    }
    if (item->type() == REPO_ITEM_TYPE) {
        return;
    } else {
        // A repo category item
        if (isExpanded(index)) {
            collapse(index);
        } else {
            expand(index);
        }
    }
}

void RepoTreeView::viewRepoOnWeb()
{
    QString repo_id = view_on_web_action_->data().toString();
    const Account& account = cloud_view_->currentAccount();
    if (account.isValid()) {
        QUrl url = account.serverUrl;
        url.setPath(url.path() + "/repo/" + repo_id);
        QDesktopServices::openUrl(url);
    }
}

bool RepoTreeView::viewportEvent(QEvent *event)
{
    if (event->type() != QEvent::ToolTip && event->type() != QEvent::WhatsThis) {
        return QTreeView::viewportEvent(event);
    }

    QPoint global_pos = QCursor::pos();
    QPoint viewport_pos = viewport()->mapFromGlobal(global_pos);
    QModelIndex index = indexAt(viewport_pos);
    if (!index.isValid()) {
        return true;
    }

    QStandardItem *item = getRepoItem(index);
    if (!item) {
        return true;
    }

    QRect item_rect = visualRect(index);
    if (item->type() == REPO_ITEM_TYPE) {
        showRepoItemToolTip((RepoItem *)item, global_pos, item_rect);
    } else {
        showRepoCategoryItemToolTip((RepoCategoryItem *)item, global_pos, item_rect);
    }

    return true;
}

void RepoTreeView::showRepoItemToolTip(const RepoItem *item,
                                       const QPoint& pos,
                                       const QRect& rect)
{
    RepoItemDelegate *delegate = (RepoItemDelegate *)itemDelegate();
    delegate->showRepoItemToolTip(item, pos, viewport(), rect);
}

void RepoTreeView::showRepoCategoryItemToolTip(const RepoCategoryItem *item,
                                               const QPoint& pos,
                                               const QRect& rect)
{
    QToolTip::showText(pos, item->name(), viewport(), rect);
    // QToolTip::showText(pos, item->name());
}

std::vector<QAction*> RepoTreeView::getToolBarActions()
{
    std::vector<QAction*> actions;

    actions.push_back(download_action_);
    actions.push_back(open_local_folder_action_);
    actions.push_back(view_on_web_action_);
    actions.push_back(show_detail_action_);
    return actions;
}

void RepoTreeView::selectionChanged(const QItemSelection &selected,
                                    const QItemSelection &deselected)
{
    updateRepoActions();
}

void RepoTreeView::hideEvent(QHideEvent *event)
{
    download_action_->setEnabled(false);
    open_local_folder_action_->setEnabled(false);
    toggle_auto_sync_action_->setEnabled(false);
    view_on_web_action_->setEnabled(false);
}

void RepoTreeView::showEvent(QShowEvent *event)
{
    updateRepoActions();
}
