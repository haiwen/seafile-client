#include <QtGui>
#include <QHeaderView>
#include <QPainter>

#include "QtAwesome.h"
#include "utils/utils.h"
#include "seafile-applet.h"
#include "rpc/rpc-client.h"
#include "rpc/local-repo.h"
#include "sync-repo-dialog.h"
#include "repo-item.h"
#include "repo-tree-model.h"
#include "repo-tree-view.h"

RepoTreeView::RepoTreeView(QWidget *parent)
    : QTreeView(parent)
{
    header()->hide();
    createContextMenu();

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

    prepareContextMenu((RepoItem *)item);
    pos = viewport()->mapToGlobal(pos);
    context_menu_->exec(pos);
}

void RepoTreeView::prepareContextMenu(const RepoItem *item)
{
    if (item->localRepo().isValid()) {
        download_action_->setVisible(false);
        download_action_->setData(QVariant::fromValue(item->repo()));

        open_local_folder_action_->setData(QVariant::fromValue(item->localRepo()));
        open_local_folder_action_->setVisible(true);
    } else {
        download_action_->setVisible(true);
        open_local_folder_action_->setVisible(false);
    }

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

void RepoTreeView::createContextMenu()
{
    context_menu_ = new QMenu(this);

    show_detail_action_ = new QAction(tr("&Show details"), this);
    show_detail_action_->setIcon(awesome->icon(icon_info_sign));
    show_detail_action_->setStatusTip(tr("Download this library"));
    connect(show_detail_action_, SIGNAL(triggered()), this, SLOT(showRepoDetail()));

    // context_menu_->addAction(show_detail_action_);

    download_action_ = new QAction(tr("&Download this library"), this);
    download_action_->setIcon(awesome->icon(icon_download));
    download_action_->setStatusTip(tr("Download this library"));
    connect(download_action_, SIGNAL(triggered()), this, SLOT(downloadRepo()));

    context_menu_->addAction(download_action_);

    open_local_folder_action_ = new QAction(tr("&Open folder"), this);
    open_local_folder_action_->setIcon(awesome->icon(icon_folder_open_alt));
    open_local_folder_action_->setStatusTip(tr("open this folder with file manager"));
    connect(open_local_folder_action_, SIGNAL(triggered()), this, SLOT(openLocalFolder()));

    context_menu_->addAction(open_local_folder_action_);
}

void RepoTreeView::downloadRepo()
{
    ServerRepo repo = qvariant_cast<ServerRepo>(download_action_->data());
    SyncRepoDialog dialog(repo, this);
    dialog.exec();
}

void RepoTreeView::showRepoDetail()
{
}

void RepoTreeView::openLocalFolder()
{
    LocalRepo repo = qvariant_cast<LocalRepo>(open_local_folder_action_->data());
    open_dir(repo.worktree);
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
      
