#include <QtGlobal>

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
#include <QtWidgets>
#else
#include <QtGui>
#endif
#include <QHeaderView>
#include <QDesktopServices>
#include <QEvent>
#include <QShowEvent>
#include <QHideEvent>
#include <QInputDialog>
#include <Qt>

#include "utils/utils.h"
#include "seafile-applet.h"
#include "settings-mgr.h"
#include "account-mgr.h"
#include "rpc/rpc-client.h"
#include "rpc/local-repo.h"
#include "download-repo-dialog.h"
#include "clone-tasks-dialog.h"
#include "repo-item.h"
#include "repo-item-delegate.h"
#include "repo-tree-model.h"
#include "repo-detail-dialog.h"
#include "utils/paint-utils.h"
#include "repo-service.h"
#include "auto-login-service.h"

#include "filebrowser/file-browser-manager.h"
#include "filebrowser/file-browser-dialog.h"
#include "utils/utils-mac.h"
#include "filebrowser/tasks.h"
#include "filebrowser/progress-dialog.h"
#include "ui/set-repo-password-dialog.h"
#include "ui/private-share-dialog.h"
#include "ui/check-repo-root-perm-dialog.h"

#include "repo-tree-view.h"

namespace {

const char *kRepoTreeViewSettingsGroup = "RepoTreeView";
const char *kRepoTreeViewSettingsExpandedCategories = "expandedCategories";
const char *kSyncIntervalProperty = "sync-interval";

// A Helper Class to copy file
//
class FileCopyHelper : public QRunnable {
public:
    FileCopyHelper(const QString &source, const QString &target,
                   RepoTreeView *parent)
    : source_(source),
      target_(target),
      parent_(parent) {
    }
signals:
    void success(bool);
private:
    void run() {
        if (!QFile::copy(source_, target_)) {
            // cannot do GUI operations in this thread
            // callback to the main thread
            QMetaObject::invokeMethod(parent_, "copyFileFailed");
        }
    }
    bool autoDelete() {
        return true;
    }
    const QString source_;
    const QString target_;
    RepoTreeView *parent_;
};

FileCopyHelper* copyFile(const QString &source, const QString &target, RepoTreeView *parent) {
      FileCopyHelper* helper = new FileCopyHelper(source, target, parent);
      QThreadPool *pool = QThreadPool::globalInstance();
      pool->start(helper);
      return helper;
}

} // anonymous namespace

static ServerRepo selected_repo_;
// TODO save localrepo as well to avoid many copys

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
#if defined(Q_OS_MAC)
    this->setAttribute(Qt::WA_MacShowFocusRect, 0);
#endif

    loadExpandedCategries();
    connect(qApp, SIGNAL(aboutToQuit()),
            this, SLOT(saveExpandedCategries()));
    connect(seafApplet->accountManager(), SIGNAL(beforeAccountSwitched()),
            this, SLOT(saveExpandedCategries()));
    connect(seafApplet->accountManager(), SIGNAL(accountsChanged()),
            this, SLOT(loadExpandedCategries()));
    connect(seafApplet->settingsManager(), SIGNAL(autoSyncChanged(bool)),
            this, SLOT(update()));

    setAcceptDrops(true);
    setDefaultDropAction(Qt::CopyAction);
    // setAlternatingRowColors(true);
    setDropIndicatorShown(true);

    current_drop_target_ = QModelIndex();
    previous_drop_target_ = QModelIndex();
}

void RepoTreeView::loadExpandedCategries()
{
    Account account = seafApplet->accountManager()->currentAccount();
    if (!account.isValid()) {
        return;
    }
    expanded_categroies_.clear();
    QSettings settings;
    settings.beginGroup(kRepoTreeViewSettingsGroup);
    QString key = QString(kRepoTreeViewSettingsExpandedCategories) + "-" + account.getSignature();
    if (settings.contains(key)) {
        QString cats = settings.value(key, "").toString();
        expanded_categroies_ = QSet<QString>::fromList(cats.split("\t", QString::SkipEmptyParts));
    } else {
        // Expand "recent updated" on first use
        expanded_categroies_.insert(tr("Recently Updated"));
    }
    settings.endGroup();
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
    menu->deleteLater();
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
    menu->addAction(open_in_filebrowser_action_);

    if (item->repo().isSharedRepo()) {
        menu->addAction(unshare_action_);
    }

    const Account& account = seafApplet->accountManager()->currentAccount();
    if (account.isPro() && account.username == item->repo().owner) {
        menu->addSeparator();
        menu->addAction(share_repo_to_user_action_);
        menu->addAction(share_repo_to_group_action_);
        menu->addSeparator();
    }

    if (item->localRepo().isValid()) {
        menu->addSeparator();
        menu->addAction(toggle_auto_sync_action_);
        menu->addAction(sync_now_action_);
        menu->addAction(set_sync_interval_action_);
        menu->addSeparator();
    }

    menu->addAction(show_detail_action_);

    if (item->cloneTask().isCancelable()) {
        menu->addAction(cancel_download_action_);
    }
    if (item->localRepo().isValid()) {
        menu->addAction(unsync_action_);
        menu->addAction(resync_action_);
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
        QSortFilterProxyModel *proxy = (QSortFilterProxyModel *)model();
        RepoTreeModel *tree_model = (RepoTreeModel *)(proxy->sourceModel());
        QStandardItem *it = tree_model->itemFromIndex(proxy->mapToSource(index));
        if (it && it->type() == REPO_ITEM_TYPE) {
            item = (RepoItem *)it;
        }
    }

    if (!item) {
        // No repo item is selected
        download_action_->setEnabled(false);
        download_toolbar_action_->setEnabled(false);
        sync_now_action_->setEnabled(false);
        open_local_folder_action_->setEnabled(false);
        open_local_folder_toolbar_action_->setEnabled(false);
        unsync_action_->setEnabled(false);
        resync_action_->setEnabled(false);
        set_sync_interval_action_->setEnabled(false);
        toggle_auto_sync_action_->setEnabled(false);
        view_on_web_action_->setEnabled(false);
        open_in_filebrowser_action_->setEnabled(false);
        show_detail_action_->setEnabled(false);
        return;
    }

    LocalRepo r;
    seafApplet->rpcClient()->getLocalRepo(item->repo().id, &r);
    item->setLocalRepo(r);

    if (item->localRepo().isValid()) {
        const LocalRepo& local_repo = item->localRepo();
        download_action_->setEnabled(false);
        download_toolbar_action_->setEnabled(false);

        sync_now_action_->setEnabled(true);
        sync_now_action_->setData(QVariant::fromValue(local_repo));

        open_local_folder_action_->setData(QVariant::fromValue(local_repo));
        open_local_folder_action_->setEnabled(true);
        open_local_folder_toolbar_action_->setData(QVariant::fromValue(local_repo));
        open_local_folder_toolbar_action_->setEnabled(true);

        unsync_action_->setData(QVariant::fromValue(local_repo));
        unsync_action_->setEnabled(true);

        resync_action_->setData(QVariant::fromValue(local_repo));
        resync_action_->setEnabled(true);

        set_sync_interval_action_->setData(QVariant::fromValue(local_repo));
        set_sync_interval_action_->setEnabled(true);

        if (seafApplet->settingsManager()->autoSync()) {
            toggle_auto_sync_action_->setData(QVariant::fromValue(local_repo));
            toggle_auto_sync_action_->setEnabled(true);
        } else {
            toggle_auto_sync_action_->setEnabled(false);
        }

        if (local_repo.auto_sync) {
            toggle_auto_sync_action_->setText(tr("Disable auto sync"));
            toggle_auto_sync_action_->setToolTip(tr("Disable auto sync"));
            toggle_auto_sync_action_->setIcon(QIcon(":/images/pause-gray.png"));
        } else {
            toggle_auto_sync_action_->setText(tr("Enable auto sync"));
            toggle_auto_sync_action_->setToolTip(tr("Enable auto sync"));
            toggle_auto_sync_action_->setIcon(QIcon(":/images/play-gray.png"));
        }

    } else {
        if (item->repoDownloadable()) {
            download_action_->setEnabled(true);
            download_toolbar_action_->setEnabled(true);
        } else {
            download_action_->setEnabled(false);
            download_toolbar_action_->setEnabled(false);
        }

        sync_now_action_->setEnabled(false);

        open_local_folder_action_->setEnabled(false);
        open_local_folder_toolbar_action_->setEnabled(false);
        unsync_action_->setEnabled(false);
        resync_action_->setEnabled(false);
        set_sync_interval_action_->setEnabled(false);
        toggle_auto_sync_action_->setEnabled(false);
    }

    selected_repo_ = item->repo();
    view_on_web_action_->setEnabled(true);
    open_in_filebrowser_action_->setEnabled(true);

    show_detail_action_->setEnabled(true);

    if (item->cloneTask().isCancelable()) {
        cancel_download_action_->setEnabled(true);
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
    QSortFilterProxyModel *proxy = (QSortFilterProxyModel *)model();
    RepoTreeModel *tree_model = (RepoTreeModel *)(proxy->sourceModel());
    const QModelIndex mapped_index = proxy->mapToSource(index);
    QStandardItem *item = tree_model->itemFromIndex(mapped_index);

    if (item->type() != REPO_ITEM_TYPE &&
        item->type() != REPO_CATEGORY_TYPE) {
        return NULL;
    }
    return item;
}

void RepoTreeView::createActions()
{
    show_detail_action_ = new QAction(tr("Show &details"), this);
    show_detail_action_->setIcon(QIcon(":/images/info-gray.png"));
    show_detail_action_->setStatusTip(tr("Show details of this library"));
    show_detail_action_->setIconVisibleInMenu(true);
    connect(show_detail_action_, SIGNAL(triggered()), this, SLOT(showRepoDetail()));

    download_action_ = new QAction(tr("&Sync this library"), this);
    download_action_->setIcon(QIcon(":/images/toolbar/download-gray.png"));
    download_action_->setStatusTip(tr("Sync this library"));
    download_action_->setIconVisibleInMenu(true);
    connect(download_action_, SIGNAL(triggered()), this, SLOT(downloadRepo()));

    download_toolbar_action_ = new QAction(tr("&Sync this library"), this);
    download_toolbar_action_->setIcon(QIcon(":/images/toolbar/download.png"));
    download_toolbar_action_->setStatusTip(tr("Sync this library"));
    download_toolbar_action_->setIconVisibleInMenu(false);
    connect(download_toolbar_action_, SIGNAL(triggered()), this, SLOT(downloadRepo()));

    sync_now_action_ = new QAction(tr("Sync &now"), this);
    sync_now_action_->setIcon(QIcon(":/images/sync_now-gray.png"));
    sync_now_action_->setStatusTip(tr("Sync this library immediately"));
    sync_now_action_->setIconVisibleInMenu(true);
    connect(sync_now_action_, SIGNAL(triggered()), this, SLOT(syncRepoImmediately()));
    cancel_download_action_ = new QAction(tr("&Cancel download"), this);
    cancel_download_action_->setIcon(QIcon(":/images/remove-gray.png"));
    cancel_download_action_->setStatusTip(tr("Cancel download of this library"));
    cancel_download_action_->setIconVisibleInMenu(true);
    connect(cancel_download_action_, SIGNAL(triggered()), this, SLOT(cancelDownload()));

    open_local_folder_action_ = new QAction(tr("&Open local folder"), this);
    open_local_folder_action_->setIcon(QIcon(":/images/toolbar/file-gray.png"));
    open_local_folder_action_->setStatusTip(tr("open local folder"));
    open_local_folder_action_->setIconVisibleInMenu(true);
    connect(open_local_folder_action_, SIGNAL(triggered()), this, SLOT(openLocalFolder()));

    open_local_folder_toolbar_action_ = new QAction(tr("&Open folder"), this);
    open_local_folder_toolbar_action_->setIcon(QIcon(":/images/toolbar/file.png"));
    open_local_folder_toolbar_action_->setStatusTip(tr("open local folder"));
    open_local_folder_toolbar_action_->setIconVisibleInMenu(true);
    connect(open_local_folder_toolbar_action_, SIGNAL(triggered()), this, SLOT(openLocalFolder()));

    unsync_action_ = new QAction(tr("&Unsync"), this);
    unsync_action_->setStatusTip(tr("unsync this library"));
    unsync_action_->setIcon(QIcon(":/images/minus-gray.png"));
    unsync_action_->setIconVisibleInMenu(true);
    connect(unsync_action_, SIGNAL(triggered()), this, SLOT(unsyncRepo()));

    toggle_auto_sync_action_ = new QAction(tr("Enable auto sync"), this);
    toggle_auto_sync_action_->setStatusTip(tr("Enable auto sync"));
    toggle_auto_sync_action_->setIconVisibleInMenu(true);
    connect(toggle_auto_sync_action_, SIGNAL(triggered()), this, SLOT(toggleRepoAutoSync()));

    view_on_web_action_ = new QAction(tr("&View on cloud"), this);
    view_on_web_action_->setIcon(QIcon(":/images/cloud-gray.png"));
    view_on_web_action_->setStatusTip(tr("view this library on seahub"));
    view_on_web_action_->setIconVisibleInMenu(true);

    connect(view_on_web_action_, SIGNAL(triggered()), this, SLOT(viewRepoOnWeb()));

    share_repo_to_user_action_ = new QAction(tr("Share to user"), this);
    share_repo_to_user_action_->setIcon(QIcon(":/images/share.png"));
    share_repo_to_user_action_->setStatusTip(tr("Share this library to a user"));
    share_repo_to_user_action_->setIconVisibleInMenu(true);

    connect(share_repo_to_user_action_, SIGNAL(triggered()), this, SLOT(shareRepoToUser()));

    share_repo_to_group_action_ = new QAction(tr("Share to group"), this);
    share_repo_to_group_action_->setIcon(QIcon(":/images/share.png"));
    share_repo_to_group_action_->setStatusTip(tr("Share this library to a group"));
    share_repo_to_group_action_->setIconVisibleInMenu(true);

    connect(share_repo_to_group_action_, SIGNAL(triggered()), this, SLOT(shareRepoToGroup()));

    QString open_action_text = tr("&Open cloud file browser");
    if (open_action_text.contains("%")) {
        // In German translation there is a "seafile" string, so need to use tr("..").arg(..) here
        open_action_text = open_action_text.arg(getBrand());
    }

    open_in_filebrowser_action_ = new QAction(open_action_text, this);
    open_in_filebrowser_action_->setIcon(QIcon(":/images/cloud-gray.png"));
    open_in_filebrowser_action_->setStatusTip(tr("open this library in embedded Cloud File Browser"));
    open_in_filebrowser_action_->setIconVisibleInMenu(true);

    connect(open_in_filebrowser_action_, SIGNAL(triggered()), this, SLOT(openInFileBrowser()));

    unshare_action_ = new QAction(tr("&Leave share"), this);
    unshare_action_->setIcon(QIcon(":/images/leave-share.png"));
    unshare_action_->setStatusTip(tr("leave share"));

    connect(unshare_action_, SIGNAL(triggered()), this, SLOT(unshareRepo()));

    resync_action_ = new QAction(tr("&Resync this library"), this);
    resync_action_->setIcon(QIcon(":/images/resync.png"));
    resync_action_->setStatusTip(tr("unsync and resync this library"));

    connect(resync_action_, SIGNAL(triggered()), this, SLOT(resyncRepo()));

    set_sync_interval_action_ = new QAction(tr("Set sync &Interval"), this);
    set_sync_interval_action_->setIcon(QIcon(":/images/clock.png"));
    set_sync_interval_action_->setStatusTip(tr("set sync interval for this library"));

    connect(set_sync_interval_action_, SIGNAL(triggered()), this, SLOT(setRepoSyncInterval()));
}

void RepoTreeView::downloadRepo()
{
    DownloadRepoDialog dialog(seafApplet->accountManager()->currentAccount(), selected_repo_, QString(), this);

    dialog.exec();

    updateRepoActions();
}

void RepoTreeView::showRepoDetail()
{
    RepoDetailDialog dialog(selected_repo_, this);
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

    QString question = tr("Are you sure to unsync the library \"%1\"?").arg(repo.name);

    if (!seafApplet->yesOrCancelBox(question, this, false)) {
        return;
    }

    unsyncRepoImpl(repo);
}

void RepoTreeView::unsyncRepoImpl(const LocalRepo& repo)
{
    if (seafApplet->rpcClient()->unsync(repo.id) < 0) {
        seafApplet->warningBox(tr("Failed to unsync library \"%1\"").arg(repo.name), this);
    }
    ServerRepo server_repo = RepoService::instance()->getRepo(repo.id);
    if (server_repo.isValid() && server_repo.isSubfolder())
        RepoService::instance()->removeSyncedSubfolder(repo.id);

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
            // open local folder for downloaded repo
            QDesktopServices::openUrl(QUrl::fromLocalFile(local_repo.worktree));
        } else {
            // open seahub repo page for not downloaded repo
            FileBrowserManager::getInstance()->openOrActivateDialog(
                seafApplet->accountManager()->currentAccount(),
                it->repo());
        }
    }
}

void RepoTreeView::viewRepoOnWeb()
{
    const Account account = seafApplet->accountManager()->currentAccount();
    if (account.isValid()) {
        // we adopt a new format of cloud view url from server version 4.2.0
        if (!account.isAtLeastVersion(4, 2, 0)) {
            QDesktopServices::openUrl(account.getAbsoluteUrl("repo/" + selected_repo_.id));
        } else {
            AutoLoginService::instance()->startAutoLogin("/#common/lib/" + selected_repo_.id + "/");
        }
    }
}

void RepoTreeView::shareRepo(bool to_group)
{
    const Account account = seafApplet->accountManager()->currentAccount();
    PrivateShareDialog dialog(account, selected_repo_.id, selected_repo_.name,
                              "/", to_group,
                              this);
    dialog.exec();
}

void RepoTreeView::shareRepoToUser()
{
    shareRepo(false);
}

void RepoTreeView::shareRepoToGroup()
{
    shareRepo(true);
}

void RepoTreeView::unshareRepo()
{
    if (!seafApplet->yesOrNoBox(
            tr("Are you sure you want to leave the share \"%1\"?").arg(
                selected_repo_.name), this, false)) {
        return;
    }

    const Account account = seafApplet->accountManager()->currentAccount();
    const QString repo_id = selected_repo_.id;
    const QString from_user = selected_repo_.owner;
    UnshareRepoRequest* request =
        new UnshareRepoRequest(account, repo_id, from_user);

    connect(request, SIGNAL(success()),
            this, SLOT(onUnshareSuccess()));
    connect(request, SIGNAL(failed(const ApiError&)),
            this, SLOT(onUnshareFailed(const ApiError&)));

    request->send();
}

void RepoTreeView::onUnshareSuccess()
{
    RepoService::instance()->refresh(true);

    UnshareRepoRequest* req = qobject_cast<UnshareRepoRequest*>(sender());
    if (!req) {
        return;
    } else {
        req->deleteLater();
    }

    LocalRepo local_repo;
    seafApplet->rpcClient()->getLocalRepo(req->repoId(), &local_repo);
    if (local_repo.isValid()) {
        unsyncRepoImpl(local_repo);
    }
}

void RepoTreeView::onUnshareFailed(const ApiError&error)
{
    seafApplet->warningBox(tr("Leaving share failed"), this);
}

void RepoTreeView::openInFileBrowser()
{
    const Account account = seafApplet->accountManager()->currentAccount();
    if (account.isValid()) {
        FileBrowserManager::getInstance()->openOrActivateDialog(
            seafApplet->accountManager()->currentAccount(),
            selected_repo_);
    }
}

bool RepoTreeView::viewportEvent(QEvent *event)
{
    if (event->type() != QEvent::ToolTip &&
        event->type() != QEvent::WhatsThis &&
        event->type() != QEvent::MouseButtonPress &&
        event->type() != QEvent::MouseButtonRelease)
    {
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

    // handle the event in the top
    const QModelIndex top_index = indexAt(QPoint(0, 0));
    if (event->type() == QEvent::MouseButtonPress ||
        event->type() == QEvent::MouseButtonRelease)
    {
        QSortFilterProxyModel *proxy = (QSortFilterProxyModel *)model();
        RepoTreeModel *tree_model = (RepoTreeModel *)(proxy->sourceModel());

        if (index == top_index &&
            top_index.parent().isValid() &&
            viewport_pos.y() <= tree_model->repo_category_height)
        {
            QMouseEvent *ev = static_cast<QMouseEvent*>(event);
            if (!(ev->buttons() & Qt::LeftButton)) {
                return true;
            } else {
                const QModelIndex parent = top_index.parent();
                setExpanded(parent, !isExpanded(parent));
                return true;
            }
        }
        return QTreeView::viewportEvent(event);
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

    actions.push_back(download_toolbar_action_);
    actions.push_back(open_local_folder_toolbar_action_);
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
    download_toolbar_action_->setEnabled(false);
    open_local_folder_action_->setEnabled(false);
    open_local_folder_toolbar_action_->setEnabled(false);
    unsync_action_->setEnabled(false);
    resync_action_->setEnabled(false);
    set_sync_interval_action_->setEnabled(false);
    toggle_auto_sync_action_->setEnabled(false);
    view_on_web_action_->setEnabled(false);
    open_in_filebrowser_action_->setEnabled(false);
    show_detail_action_->setEnabled(false);
}

void RepoTreeView::saveExpandedCategries()
{
    Account account = seafApplet->accountManager()->currentAccount();
    if (!account.isValid()) {
        return;
    }
    QSettings settings;
    QStringList cats = expanded_categroies_.toList();
    settings.beginGroup(kRepoTreeViewSettingsGroup);
    QString key = QString(kRepoTreeViewSettingsExpandedCategories) + "-" + account.getSignature();
    settings.setValue(key, cats.join("\t"));
    settings.endGroup();
}

void RepoTreeView::showEvent(QShowEvent *event)
{
    updateRepoActions();
}

void RepoTreeView::syncRepoImmediately()
{
    LocalRepo repo = qvariant_cast<LocalRepo>(sync_now_action_->data());

    seafApplet->rpcClient()->syncRepoImmediately(repo.id);

    QSortFilterProxyModel *proxy = (QSortFilterProxyModel *)model();
    RepoTreeModel *tree_model = (RepoTreeModel *)(proxy->sourceModel());
    tree_model->updateRepoItemAfterSyncNow(repo.id);
}

void RepoTreeView::cancelDownload()
{
    QString error;
    if (seafApplet->rpcClient()->cancelCloneTask(selected_repo_.id, &error) < 0) {
        seafApplet->warningBox(tr("Failed to cancel this task:\n\n %1").arg(error), this);
    } else {
        seafApplet->messageBox(tr("The download has been canceled"), this);
    }
}

void RepoTreeView::expand(const QModelIndex& index, bool remember)
{
    QTreeView::expand(index);
    if (remember) {
        QStandardItem *item = getRepoItem(index);
        if (item->type() == REPO_CATEGORY_TYPE) {
            expanded_categroies_.insert(item->data(Qt::DisplayRole).toString());
        }
    }
}

void RepoTreeView::collapse(const QModelIndex& index, bool remember)
{
    QTreeView::collapse(index);
    if (remember) {
        QStandardItem *item = getRepoItem(index);
        if (item->type() == REPO_CATEGORY_TYPE) {
            expanded_categroies_.remove(item->data(Qt::DisplayRole).toString());
        }
    }
}

void RepoTreeView::restoreExpandedCategries()
{
    QSortFilterProxyModel *proxy_model =
        (QSortFilterProxyModel *)(this->model());
    RepoTreeModel *tree_model = (RepoTreeModel *)(proxy_model->sourceModel());

    for (int i = 0; i < proxy_model->rowCount(); i++) {
        QModelIndex index = proxy_model->index(i, 0);
        QString category = proxy_model->data(index).toString();
        if (expanded_categroies_.contains(category)) {
            expand(index, false);
        } else {
            collapse(index, false);
        }

        QStandardItem *item =
            tree_model->itemFromIndex(proxy_model->mapToSource(index));

        // We need to go one level down if this item is the groups root.
        if (item->type() == REPO_CATEGORY_TYPE &&
            ((RepoCategoryItem *)item)->isGroupsRoot()) {
            for (int j = 0; j < item->rowCount(); j++) {
                RepoCategoryItem *category_item =
                    (RepoCategoryItem *)(item->child(j));
                QModelIndex index = proxy_model->mapFromSource(
                    tree_model->indexFromItem(category_item));
                QString category = proxy_model->data(index).toString();
                if (expanded_categroies_.contains(category)) {
                    expand(index, false);
                } else {
                    collapse(index, false);
                }
            }
        }
    }
}

void RepoTreeView::resyncRepo()
{
    LocalRepo local_repo = qvariant_cast<LocalRepo>(unsync_action_->data());
    ServerRepo server_repo = RepoService::instance()->getRepo(local_repo.id);

    SeafileRpcClient *rpc = seafApplet->rpcClient();

    if (!seafApplet->yesOrNoBox(
            tr("Are you sure to resync the library \"%1\"?").arg(server_repo.name),
            this)) {
        return;
    }

    if (rpc->unsync(server_repo.id) < 0) {
        seafApplet->warningBox(tr("Failed to unsync library \"%1\"").arg(server_repo.name));
        return;
    }

    DownloadRepoDialog dialog(seafApplet->accountManager()->currentAccount(),
                              RepoService::instance()->getRepo(server_repo.id), QString(), this);
    dialog.setMergeWithExisting(QFileInfo(local_repo.worktree).dir().absolutePath());
    if (!server_repo.encrypted) {
        dialog.setResyncMode();
    }

    dialog.exec();
    updateRepoActions();
}

void RepoTreeView::dropEvent(QDropEvent *event)
{
    const QModelIndex index = indexAt(event->pos());
    QStandardItem *standard_item = getRepoItem(index);
    if (!standard_item || standard_item->type() != REPO_ITEM_TYPE) {
        return;
    }
    event->accept();

    RepoItem *item = static_cast<RepoItem*>(standard_item);
    const ServerRepo &repo = item->repo();

    updateDropTarget(QModelIndex());

    const QUrl url = event->mimeData()->urls().at(0);
    QString local_path = url.toLocalFile();
#if defined(Q_OS_MAC) && (QT_VERSION <= QT_VERSION_CHECK(5, 4, 0))
        local_path = utils::mac::fix_file_id_url(local_path);
#endif

    if (repo.readonly) {
        // Do not call the `show` method of the dialog. It would show itself if
        // the task doens't finish within 4 seconds.
        //
        // This is also why we can't create the dialog object on stack and use
        // `dialog.exec()`.
        CheckRepoRootDirPermDialog *dialog = new CheckRepoRootDirPermDialog(
            seafApplet->accountManager()->currentAccount(), repo, local_path, this);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        connect(dialog, SIGNAL(finished(int)), this, SLOT(checkRootPermDone()));
    } else {
        uploadDroppedFile(repo, local_path);
    }
}

void RepoTreeView::checkRootPermDone()
{
    CheckRepoRootDirPermDialog *dialog = qobject_cast<CheckRepoRootDirPermDialog *>(sender());
    if (dialog->hasWritePerm()) {
        uploadDroppedFile(dialog->repo(), dialog->localPath());
    } else {
        seafApplet->warningBox(tr("You do not have permission to upload to this folder"));
    }
}

void RepoTreeView::uploadDroppedFile(const ServerRepo& repo, const QString& local_path)
{
    const QString file_name = QFileInfo(local_path).fileName();
    // if the repo is synced
    LocalRepo local_repo;
    if (seafApplet->rpcClient()->getLocalRepo(repo.id, &local_repo) >= 0) {
        QString target_path = QDir(local_repo.worktree).absoluteFilePath(file_name);
        if (QFileInfo(target_path) == QFileInfo(local_path)) {
            seafApplet->warningBox(tr("Unable to overwrite file \"%1\" with itself").arg(file_name));
            return;
        }

        if (QFileInfo(target_path).exists()) {
            if (!seafApplet->yesOrNoBox(tr("Are you sure to overwrite the file \"%1\"").arg(file_name)))
                return;
            if (!QFile(target_path).remove()) {
                seafApplet->warningBox(tr("Unable to delete file \"%1\"").arg(file_name));
                return;
            }
        }

        copyFile(local_path, target_path, this);

        return;
    }

    FileUploadTask *task = new FileUploadTask(seafApplet->accountManager()->currentAccount(),
          repo.id, "/", local_path, file_name);
    uploadFileStart(task);
}

void RepoTreeView::dragMoveEvent(QDragMoveEvent *event)
{
    QPoint pos = event->pos();
    const QModelIndex index = indexAt(pos);
    QRect rect = visualRect(index);

    // highlight the selected item, and dehightlight when it's over
    QStandardItem *item = getRepoItem(index);
    if (item && item->type() == REPO_ITEM_TYPE) {
        if (changeGrayBackground(pos, rect)) {
            updateDropTarget(index);
            event->setDropAction(Qt::CopyAction);
            event->accept();
        } else {
            updateDropTarget(QModelIndex());
            event->setDropAction(Qt::IgnoreAction);
            event->accept();
        }
    } else {
       event->setDropAction(Qt::IgnoreAction);
       event->accept();
       return;
    }
}

void RepoTreeView::dragLeaveEvent(QDragLeaveEvent *event)
{
    updateDropTarget(QModelIndex());
    QTreeView::dragLeaveEvent(event);
}

void RepoTreeView::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls() && event->mimeData()->urls().size() == 1) {
        const QUrl url = event->mimeData()->urls().at(0);
        if (url.scheme() == "file") {

            QString file_name = url.toLocalFile();
#if defined(Q_OS_MAC) && (QT_VERSION <= QT_VERSION_CHECK(5, 4, 0))
            file_name = utils::mac::fix_file_id_url(file_name);
#endif

            if (QFileInfo(file_name).isFile()) {
                event->setDropAction(Qt::CopyAction);
                event->accept();
            }
        }
    }
}

bool RepoTreeView::changeGrayBackground(
    const QPoint& pos, const QRect& rect) const
{
    const int margin = 2;
    if ((pos.y() - rect.top() > margin) &&
        (rect.bottom() - pos.y() > margin)) {
        return true;
    } else {
        return false;
    }
}

void RepoTreeView::updateBackground()
{
    if (current_drop_target_.isValid()) {
        dataChanged(current_drop_target_, current_drop_target_,
                    QVector<int>(1, Qt::BackgroundRole));
    }

    if (previous_drop_target_.isValid()) {
        dataChanged(previous_drop_target_, previous_drop_target_,
                    QVector<int>(1, Qt::BackgroundRole));
    }
}

void RepoTreeView::updateDropTarget(const QModelIndex& index)
{
    previous_drop_target_ = current_drop_target_;
    if (index.isValid() && index == current_drop_target_) {
        // No need to repaint since the cursor is still with in the same repo
        // item.
        return;
    }

    current_drop_target_ = index;
    updateBackground();
}

void RepoTreeView::uploadFileStart(FileUploadTask *task)
{
    connect(task, SIGNAL(finished(bool)),
            this, SLOT(uploadFileFinished(bool)));
    FileBrowserProgressDialog *dialog = new FileBrowserProgressDialog(task, this);

    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
    const QPoint global = this->mapToGlobal(rect().center());
    dialog->move(global.x() - dialog->width() / 2,
                 global.y() - dialog->height() / 2);
    dialog->raise();
    dialog->activateWindow();

    task->start();
}

void RepoTreeView::uploadFileFinished(bool success)
{
    FileUploadTask *task = qobject_cast<FileUploadTask *>(sender());
    if (task == NULL)
        return;

    if (!success) {
        // if the user cancel the task, don't bother him(or her) with it
        if (task->error() == FileNetworkTask::TaskCanceled)
            return;
        // failed and it is a encrypted repository
        ServerRepo repo = RepoService::instance()->getRepo(task->repoId());
        if (repo.encrypted && task->httpErrorCode() == 400) {
            SetRepoPasswordDialog password_dialog(repo, this);
            if (password_dialog.exec()) {
                FileUploadTask *new_task = new FileUploadTask(*task);
                uploadFileStart(new_task);
            }
            return;
        }

        QString msg = tr("Failed to upload file: %1").arg(task->errorString());
        seafApplet->warningBox(msg, this);
    }
}

void RepoTreeView::copyFileFailed()
{
    QString msg = QObject::tr("copy failed");
    seafApplet->warningBox(msg);
}

void RepoTreeView::setRepoSyncInterval()
{
    LocalRepo local_repo =
        qvariant_cast<LocalRepo>(set_sync_interval_action_->data());

    int default_interval = 0;

    QString value;
    if (seafApplet->rpcClient()->getRepoProperty(
            local_repo.id, kSyncIntervalProperty, &value) == 0) {
        default_interval = value.toInt();
    }

    QInputDialog dialog(this);
    dialog.setInputMode(QInputDialog::IntInput);
    dialog.setIntMinimum(0);
    dialog.setIntMaximum(2147483647);
    dialog.setIntStep(10);
    dialog.setIntValue(default_interval);
    dialog.setObjectName("syncIntervalDialog");
    dialog.setLabelText(tr("Sync Interval (In seconds):"));
    dialog.setWindowTitle(
        tr("Set Sync Internval For Library \"%1\"").arg(local_repo.name));
    dialog.setWindowIcon(QIcon(":/images/seafile.png"));
    dialog.setWindowFlags(dialog.windowFlags() & ~Qt::WindowContextHelpButtonHint);
    dialog.resize(400, 100);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    int interval = dialog.intValue();

    if (interval != 0 && interval == default_interval) {
        return;
    }

    seafApplet->rpcClient()->setRepoProperty(
        local_repo.id, kSyncIntervalProperty, QString::number(interval));
}
