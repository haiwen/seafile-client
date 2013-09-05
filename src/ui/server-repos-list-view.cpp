#include <QtGui>
#include <QModelIndex>

#include "QtAwesome.h"
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

    pos = viewport()->mapToGlobal(pos);
    context_menu_->exec(pos);
}

void ServerReposListView::createContextMenu()
{
    context_menu_ = new QMenu(this);
    context_menu_->addAction(download_action_);
    context_menu_->addAction(sync_action_);
    // setContextMenuPolicy(Qt::CustomContextMenu);
    // connect(this, SIGNAL(customContextMenuRequested(const QPoint &)),
    //         this, SLOT(showContextMenu(const QPoint &)));
}

void ServerReposListView::downloadRepo()
{
}

void ServerReposListView::syncRepo()
{
}
