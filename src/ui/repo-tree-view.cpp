#include <QtGui>
#include <QHeaderView>
#include <QDesktopServices>
#include <QEvent>
#include <QShowEvent>
#include <QHideEvent>
#include <Qt>

#include "QtAwesome.h"
#include "utils/utils.h"
#include "seafile-applet.h"
#include "account-mgr.h"
#include "rpc/rpc-client.h"
#include "rpc/local-repo.h"
#include "download-repo-dialog.h"
#include "clone-tasks-dialog.h"
#include "repo-item.h"
#include "repo-item-delegate.h"
#include "repo-tree-model.h"
#include "repo-tree-view.h"
#include "repo-detail-dialog.h"


RepoTreeView::RepoTreeView(QWidget *parent)
    : QTreeView(parent)
{
    header()->hide();
    createActions();

    // We draw the indicator ourselves
    setIndentation(0);
    // We handle the click oursevles
    setExpandsOnDoubleClick(false);

    connect(this, SIGNAL(clicked(const QModelIndex&)),
            this, SLOT(onItemClicked(const QModelIndex&)));

    connect(this, SIGNAL(doubleClicked(const QModelIndex&)),
            this, SLOT(onItemDoubleClicked(const QModelIndex&)));
#ifdef Q_WS_MAC
    this->setAttribute(Qt::WA_MacShowFocusRect, 0);
#endif
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
    updateRepoActions();
    QMenu *menu = prepareContextMenu((RepoItem *)item);
    pos = viewport()->mapToGlobal(pos);
    menu->exec(pos);
}

QMenu* RepoTreeView::prepareContextMenu(const RepoItem *item)
{
    QMenu *menu = new QMenu(this);
    if (item->localRepo().isValid()) {
        menu->addAction(open_local_folder_action_);
    }

    if (item->repoDownloadable()) {
        menu->addAction(download_action_);
    }

    menu->addAction(view_on_web_action_);

    if (item->localRepo().isValid()) {
        menu->addSeparator();
        menu->addAction(toggle_auto_sync_action_);
        menu->addAction(sync_now_action_);
        menu->addSeparator();
    }

    menu->addAction(show_detail_action_);

    if (item->cloneTask().isCancelable()) {
        menu->addAction(cancel_download_action_);
    }
    if (item->localRepo().isValid()) {
        menu->addAction(unsync_action_);
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
        sync_now_action_->setEnabled(false);
        open_local_folder_action_->setEnabled(false);
        unsync_action_->setEnabled(false);
        toggle_auto_sync_action_->setEnabled(false);
        view_on_web_action_->setEnabled(false);
        show_detail_action_->setEnabled(false);
        return;
    }

    LocalRepo r;
    seafApplet->rpcClient()->getLocalRepo(item->repo().id, &r);
    item->setLocalRepo(r);

    if (item->localRepo().isValid()) {
        const LocalRepo& local_repo = item->localRepo();
        download_action_->setEnabled(false);

        sync_now_action_->setEnabled(true);
        sync_now_action_->setData(QVariant::fromValue(local_repo));

        open_local_folder_action_->setData(QVariant::fromValue(local_repo));
        open_local_folder_action_->setEnabled(true);

        unsync_action_->setData(QVariant::fromValue(local_repo));
        unsync_action_->setEnabled(true);

        toggle_auto_sync_action_->setData(QVariant::fromValue(local_repo));
        toggle_auto_sync_action_->setEnabled(true);
        if (local_repo.auto_sync) {
            toggle_auto_sync_action_->setText(tr("Disable auto sync"));
            toggle_auto_sync_action_->setToolTip(tr("Disable auto sync"));
            toggle_auto_sync_action_->setIcon(QIcon(":/images/pause.png"));
        } else {
            toggle_auto_sync_action_->setText(tr("Enable auto sync"));
            toggle_auto_sync_action_->setToolTip(tr("Enable auto sync"));
            toggle_auto_sync_action_->setIcon(QIcon(":/images/play.png"));
        }

    } else {
        if (item->repoDownloadable()) {
            download_action_->setEnabled(true);
            download_action_->setData(QVariant::fromValue(item->repo()));
        } else {
            download_action_->setEnabled(false);
        }

        sync_now_action_->setEnabled(false);

        open_local_folder_action_->setEnabled(false);
        unsync_action_->setEnabled(false);
        toggle_auto_sync_action_->setEnabled(false);
    }

    view_on_web_action_->setEnabled(true);
    view_on_web_action_->setData(item->repo().id);
    show_detail_action_->setEnabled(true);
    show_detail_action_->setData(QVariant::fromValue(item->repo()));

    if (item->cloneTask().isCancelable()) {
        cancel_download_action_->setEnabled(true);
        cancel_download_action_->setData(QVariant::fromValue(item->repo()));
    } else {
        cancel_download_action_->setEnabled(false);
    }
    emit dataChanged(indexes.at(0), indexes.at(0));
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
    show_detail_action_ = new QAction(tr("Show &Details"), this);
    show_detail_action_->setIcon(QIcon(":/images/info.png"));
    show_detail_action_->setStatusTip(tr("Show details of this library"));
    show_detail_action_->setIconVisibleInMenu(true);
    connect(show_detail_action_, SIGNAL(triggered()), this, SLOT(showRepoDetail()));

    download_action_ = new QAction(tr("&Sync this library"), this);
    download_action_->setIcon(QIcon(":/images/download.png"));
    download_action_->setStatusTip(tr("Sync this library"));
    download_action_->setIconVisibleInMenu(true);
    connect(download_action_, SIGNAL(triggered()), this, SLOT(downloadRepo()));

    sync_now_action_ = new QAction(tr("Sync &Now"), this);
    sync_now_action_->setIcon(QIcon(":/images/sync_now.png"));
    sync_now_action_->setStatusTip(tr("Sync this library immediately"));
    sync_now_action_->setIconVisibleInMenu(true);
    connect(sync_now_action_, SIGNAL(triggered()), this, SLOT(syncRepoImmediately()));

    cancel_download_action_ = new QAction(tr("&Cancel download"), this);
    cancel_download_action_->setIcon(QIcon(":/images/remove.png"));
    cancel_download_action_->setStatusTip(tr("Cancel download of this library"));
    cancel_download_action_->setIconVisibleInMenu(true);
    connect(cancel_download_action_, SIGNAL(triggered()), this, SLOT(cancelDownload()));

    open_local_folder_action_ = new QAction(tr("&Open folder"), this);
    open_local_folder_action_->setIcon(QIcon(":/images/folder-open.png"));
    open_local_folder_action_->setStatusTip(tr("open local folder"));
    open_local_folder_action_->setIconVisibleInMenu(true);
    connect(open_local_folder_action_, SIGNAL(triggered()), this, SLOT(openLocalFolder()));

    unsync_action_ = new QAction(tr("&Unsync"), this);
    unsync_action_->setStatusTip(tr("unsync this library"));
    unsync_action_->setIcon(QIcon(":/images/minus.png"));
    unsync_action_->setIconVisibleInMenu(true);
    connect(unsync_action_, SIGNAL(triggered()), this, SLOT(unsyncRepo()));

    toggle_auto_sync_action_ = new QAction(tr("Enable auto sync"), this);
    toggle_auto_sync_action_->setStatusTip(tr("Enable auto sync"));
    toggle_auto_sync_action_->setIconVisibleInMenu(true);
    connect(toggle_auto_sync_action_, SIGNAL(triggered()), this, SLOT(toggleRepoAutoSync()));

    view_on_web_action_ = new QAction(tr("&View on cloud"), this);
    view_on_web_action_->setIcon(QIcon(":/images/cloud.png"));
    view_on_web_action_->setStatusTip(tr("view this library on seahub"));
    view_on_web_action_->setIconVisibleInMenu(true);

    connect(view_on_web_action_, SIGNAL(triggered()), this, SLOT(viewRepoOnWeb()));
}

void RepoTreeView::downloadRepo()
{
    ServerRepo repo = qvariant_cast<ServerRepo>(download_action_->data());
    DownloadRepoDialog dialog(seafApplet->accountManager()->accounts()[0], repo, this);

    dialog.exec();

    updateRepoActions();
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
    LocalRepo repo = qvariant_cast<LocalRepo>(toggle_auto_sync_action_->data());

    seafApplet->rpcClient()->setRepoAutoSync(repo.id, !repo.auto_sync);

    updateRepoActions();
}

void RepoTreeView::unsyncRepo()
{
    LocalRepo repo = qvariant_cast<LocalRepo>(toggle_auto_sync_action_->data());

    QString question = tr("Are you sure to unsync library \"%1\"?").arg(repo.name);

    if (QMessageBox::question(this,
                              getBrand(),
                              question,
                              QMessageBox::Ok | QMessageBox::Cancel,
                              QMessageBox::Cancel) != QMessageBox::Ok) {
        return;
    }

    if (seafApplet->rpcClient()->unsync(repo.id) < 0) {
        QMessageBox::warning(this, getBrand(),
                             tr("Failed to unsync library \"%1\"").arg(repo.name),
                             QMessageBox::Ok);
    }

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

void RepoTreeView::onItemDoubleClicked(const QModelIndex& index)
{
    QStandardItem *item = getRepoItem(index);
    if (!item) {
        return;
    }
    if (item->type() == REPO_ITEM_TYPE) {
        RepoItem *it = (RepoItem *)item;
        const LocalRepo& local_repo = it->localRepo();
        if (local_repo.isValid()) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(local_repo.worktree));
        }
    }
}

void RepoTreeView::viewRepoOnWeb()
{
    QString repo_id = view_on_web_action_->data().toString();
    const Account& account = seafApplet->accountManager()->accounts()[0];
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

    updateRepoActions();

    actions.push_back(download_action_);
    actions.push_back(open_local_folder_action_);
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
    unsync_action_->setEnabled(false);
    toggle_auto_sync_action_->setEnabled(false);
    view_on_web_action_->setEnabled(false);
    show_detail_action_->setEnabled(false);
}

void RepoTreeView::showEvent(QShowEvent *event)
{
    updateRepoActions();
}

void RepoTreeView::syncRepoImmediately()
{
    LocalRepo repo = qvariant_cast<LocalRepo>(sync_now_action_->data());

    seafApplet->rpcClient()->syncRepoImmediately(repo.id);

    ((RepoTreeModel *)model())->updateRepoItemAfterSyncNow(repo.id);
}

void RepoTreeView::cancelDownload()
{
    ServerRepo repo = qvariant_cast<ServerRepo>(cancel_download_action_->data());

    QString error;
    if (seafApplet->rpcClient()->cancelCloneTask(repo.id, &error) < 0) {
        QMessageBox::warning(this, getBrand(),
                             tr("Failed to cancel this task:\n\n %1").arg(error),
                             QMessageBox::Ok);
    } else {
        QMessageBox::information(this, getBrand(),
                                 tr("The download has been canceled"),
                                 QMessageBox::Ok);
    }
}
