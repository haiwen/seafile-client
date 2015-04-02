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
#include <Qt>

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
#include "repo-detail-dialog.h"
#include "utils/paint-utils.h"
#include "repo-service.h"

#include "filebrowser/file-browser-manager.h"
#include "filebrowser/file-browser-dialog.h"
#include "utils/utils-mac.h"
#include "filebrowser/tasks.h"
#include "filebrowser/progress-dialog.h"
#include "ui/set-repo-password-dialog.h"

#include "repo-tree-view.h"

namespace {

const int kRepoTreeMenuIconSize = 16;
const int kRepoTreeToolbarIconSize = 24;

const char *kRepoTreeViewSettingsGroup = "RepoTreeView";
const char *kRepoTreeViewSettingsExpandedCategories = "expandedCategories";

QString buildMoreInfo(ServerRepo& repo, const QUrl& url_in)
{
    json_t *object = NULL;
    char *info = NULL;

    object = json_object();
    json_object_set_new(object, "is_readonly", json_integer(repo.readonly));

    QUrl url(url_in);
    url.setPath("/");
    json_object_set_new(
        object, "server_url", json_string(url.toString().toUtf8().data()));

    info = json_dumps(object, 0);
    QString ret = QString::fromUtf8(info);
    json_decref (object);
    free (info);
    return ret;
}

QString getRepoProperty(const QString& repo_id, const QString& name)
{
    QString value;
    seafApplet->rpcClient()->getRepoProperty(repo_id, name, &value);
    return value;
}

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

}

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
    connect(seafApplet->accountManager(), SIGNAL(beforeAccountChanged()),
            this, SLOT(saveExpandedCategries()));
    connect(seafApplet->accountManager(), SIGNAL(accountsChanged()),
            this, SLOT(loadExpandedCategries()));

    setAcceptDrops(true);
    setDefaultDropAction(Qt::CopyAction);
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

        toggle_auto_sync_action_->setData(QVariant::fromValue(local_repo));
        toggle_auto_sync_action_->setEnabled(true);

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
            download_action_->setData(QVariant::fromValue(item->repo()));
            download_toolbar_action_->setData(QVariant::fromValue(item->repo()));
        } else {
            download_action_->setEnabled(false);
            download_toolbar_action_->setEnabled(false);
        }

        sync_now_action_->setEnabled(false);

        open_local_folder_action_->setEnabled(false);
        open_local_folder_toolbar_action_->setEnabled(false);
        unsync_action_->setEnabled(false);
        resync_action_->setEnabled(false);
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
    QSortFilterProxyModel *proxy = (QSortFilterProxyModel *)model();
    RepoTreeModel *tree_model = (RepoTreeModel *)(proxy->sourceModel());
    QStandardItem *item = tree_model->itemFromIndex(proxy->mapToSource(index));

    if (item->type() != REPO_ITEM_TYPE &&
        item->type() != REPO_CATEGORY_TYPE) {
        return NULL;
    }
    return item;
}

void RepoTreeView::createActions()
{
    show_detail_action_ = new QAction(tr("Show &Details"), this);
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

    sync_now_action_ = new QAction(tr("Sync &Now"), this);
    sync_now_action_->setIcon(QIcon(":/images/sync_now-gray.png"));
    sync_now_action_->setStatusTip(tr("Sync this library immediately"));
    sync_now_action_->setIconVisibleInMenu(true);
    connect(sync_now_action_, SIGNAL(triggered()), this, SLOT(syncRepoImmediately()));
    cancel_download_action_ = new QAction(tr("&Cancel download"), this);
    cancel_download_action_->setIcon(QIcon(":/images/remove-gray.png"));
    cancel_download_action_->setStatusTip(tr("Cancel download of this library"));
    cancel_download_action_->setIconVisibleInMenu(true);
    connect(cancel_download_action_, SIGNAL(triggered()), this, SLOT(cancelDownload()));

    open_local_folder_action_ = new QAction(tr("&Open folder"), this);
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

    resync_action_ = new QAction(tr("&Resync this library"), this);
    resync_action_->setIcon(QIcon(":/images/resync.png"));
    resync_action_->setStatusTip(tr("unsync and resync this library"));

    connect(resync_action_, SIGNAL(triggered()), this, SLOT(resyncRepo()));
}

void RepoTreeView::downloadRepo()
{
    ServerRepo repo = qvariant_cast<ServerRepo>(download_action_->data());
    DownloadRepoDialog dialog(seafApplet->accountManager()->currentAccount(), repo, this);

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
    QString repo_id = view_on_web_action_->data().toString();
    const Account account = seafApplet->accountManager()->currentAccount();
    if (account.isValid()) {
        QDesktopServices::openUrl(account.getAbsoluteUrl("repo/" + repo_id));
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
    toggle_auto_sync_action_->setEnabled(false);
    view_on_web_action_->setEnabled(false);
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
    QAbstractItemModel *model = this->model();
    for (int i = 0; i < model->rowCount(); i++) {
        QModelIndex index = model->index(i, 0);
        QString category = model->data(index).toString();
        if (expanded_categroies_.contains(category)) {
            expand(index, false);
        } else {
            collapse(index, false);
        }
    }
}

void RepoTreeView::resyncRepo()
{
    LocalRepo local_repo = qvariant_cast<LocalRepo>(unsync_action_->data());
    ServerRepo server_repo = RepoService::instance()->getRepo(local_repo.id);

    SeafileRpcClient *rpc = seafApplet->rpcClient();

    if (!seafApplet->yesOrNoBox(
            tr("Are you sure to unsync and resync library \"%1\"?").arg(server_repo.name),
            this)) {
        return;
    }

    // must read these before unsync
    QString token = getRepoProperty(local_repo.id, "token");
    QString relay_addr = getRepoProperty(local_repo.id, "relay-address");
    QString relay_port = getRepoProperty(local_repo.id, "relay-port");

    if (rpc->unsync(server_repo.id) < 0) {
        seafApplet->warningBox(tr("Failed to unsync library \"%1\"").arg(server_repo.name));
        return;
    }

    if (server_repo.encrypted) {
        DownloadRepoDialog dialog(seafApplet->accountManager()->currentAccount(),
                                  RepoService::instance()->getRepo(server_repo.id), this);
        dialog.setMergeWithExisting(local_repo.worktree);
        dialog.exec();
        return;
    } else {
        const Account account = seafApplet->accountManager()->currentAccount();
        QString more_info = buildMoreInfo(server_repo, account.serverUrl);
        QString email = account.username;
        QString error;

        // unused fields
        QString magic, passwd, random_key; // all null
        int enc_version = 0;
        if (rpc->cloneRepo(local_repo.id,
                           local_repo.version, local_repo.relay_id,
                           server_repo.name, local_repo.worktree,
                           token, passwd,
                           magic, relay_addr,
                           relay_port, email,
                           random_key, enc_version,
                           more_info, &error) < 0) {
            seafApplet->warningBox(tr("Failed to add download task:\n %1").arg(error));
        }
    }

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
    const QUrl url = event->mimeData()->urls().at(0);

    QString local_path = url.toLocalFile();
#if defined(Q_OS_MAC) && (QT_VERSION <= QT_VERSION_CHECK(5, 4, 0))
        local_path = utils::mac::fix_file_id_url(local_path);
#endif
    const QString file_name = QFileInfo(local_path).fileName();


    // if the repo is synced
    LocalRepo local_repo;
    if (seafApplet->rpcClient()->getLocalRepo(repo.id, &local_repo) >= 0) {
        QString target_path = QDir(local_repo.worktree).absoluteFilePath(file_name);

        if (QFileInfo(target_path).exists()) {
            if (!seafApplet->yesOrNoBox(tr("Are you sure to overwrite file \"%1\"").arg(file_name)))
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
    DropIndicatorPosition position = dropIndicatorPosition();
    if (position == QAbstractItemView::OnItem) {
        event->setDropAction(Qt::CopyAction);
        event->accept();
        //TODO highlight the selected item, and dehightlight when it's over
        // const QModelIndex index = indexAt(event->pos());
        // RepoItem *item = static_cast<RepoItem*>(getRepoItem(index));
        // if (!item || item->type() != REPO_ITEM_TYPE) {
        //     return;
        // }
    } else {
        event->setDropAction(Qt::IgnoreAction);
        event->accept();
    }
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
