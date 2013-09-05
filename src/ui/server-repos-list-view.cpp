#include <QtGui>
#include <QModelIndex>

#include "seafile-applet.h"
#include "QtAwesome.h"
#include "utils/utils.h"
#include "rpc/rpc-client.h"
#include "rpc/local-repo.h"
#include "sync-repo-dialog.h"
#include "server-repos-list-model.h"
#include "server-repos-list-view.h"


ServerReposListView::ServerReposListView(QWidget *parent): QListView(parent)
{
    createActions();
    createContextMenu();
}

void ServerReposListView::createActions()
{
    download_action_ = new QAction(tr("&Download this library"), this);
    download_action_->setIcon(awesome->icon(icon_download));
    download_action_->setStatusTip(tr("Download this library"));
    connect(download_action_, SIGNAL(triggered()), this, SLOT(downloadRepo()));

    sync_action_ = new QAction(tr("&Sync with an existing folder"), this);
    sync_action_->setIcon(awesome->icon(icon_folder_close_alt));
    sync_action_->setStatusTip(tr("Sync this library with an existing folder"));
    connect(sync_action_, SIGNAL(triggered()), this, SLOT(syncRepo()));
}

void ServerReposListView::contextMenuEvent(QContextMenuEvent *event)
{
    QPoint pos = event->pos();
    QModelIndex index = indexAt(pos);
    if (!index.isValid()) {
        // Not clicked at a repo item
        return;
    }

    prepareContextMenu(index);
    pos = viewport()->mapToGlobal(pos);
    context_menu_->exec(pos);
}

bool ServerReposListView::hasLocalRepo(const QString& repo_id)
{
    LocalRepo repo;
    if (seafApplet->rpcClient()->getLocalRepo(toCStr(repo_id), &repo) < 0) {
        return false;
    }

    return true;
}

void ServerReposListView::createContextMenu()
{
    context_menu_ = new QMenu(this);

    show_detail_action_ = new QAction(tr("&Show details"), this);
    show_detail_action_->setIcon(awesome->icon(icon_info_sign));
    show_detail_action_->setStatusTip(tr("Download this library"));
    connect(show_detail_action_, SIGNAL(triggered()), this, SLOT(showRepoDetail()));

    context_menu_->addAction(show_detail_action_);

    download_action_ = new QAction(tr("&Download this library"), this);
    download_action_->setIcon(awesome->icon(icon_download));
    download_action_->setStatusTip(tr("Download this library"));
    connect(download_action_, SIGNAL(triggered()), this, SLOT(downloadRepo()));

    context_menu_->addAction(download_action_);

    sync_action_ = new QAction(tr("&Sync with an existing folder"), this);
    sync_action_->setIcon(awesome->icon(icon_folder_close_alt));
    sync_action_->setStatusTip(tr("Sync this library with an existing folder"));
    connect(sync_action_, SIGNAL(triggered()), this, SLOT(syncRepo()));

    context_menu_->addAction(sync_action_);
}

void ServerReposListView::prepareContextMenu(const QModelIndex& index)
{
    QVariant v = model()->data(index, ServerReposListModel::RepoRole);
    ServerRepo repo = qvariant_cast<ServerRepo>(v);
    qDebug("repo id is %s\n", toCStr(repo.id));

    if (hasLocalRepo(repo.id)) {
        download_action_->setVisible(false);
        sync_action_->setVisible(false);
    } else {
        download_action_->setVisible(true);
        sync_action_->setVisible(true);
    }

}

void ServerReposListView::downloadRepo()
{
}

void ServerReposListView::syncRepo()
{
    QVariant v = model()->data(currentIndex(), ServerReposListModel::RepoRole);
    ServerRepo repo = qvariant_cast<ServerRepo>(v);
    SyncRepoDialog dialog(repo, this);
    dialog.exec();
}

void ServerReposListView::showRepoDetail()
{
}
