#include <QTimer>
#include <QHash>
#include <QDebug>
#include <algorithm>            // std::sort

#include "api/server-repo.h"

#include "utils/utils.h"
#include "seafile-applet.h"
#include "main-window.h"
#include "rpc/rpc-client.h"
#include "rpc/clone-task.h"
#include "repo-service.h"

#include "repo-item.h"
#include "repo-tree-view.h"
#include "repo-tree-model.h"

namespace {

const int kRefreshLocalReposInterval = 1000;
const int kMaxRecentUpdatedRepos = 10;
const int kIndexOfVirtualReposCategory = 2;

bool compareRepoByTimestamp(const ServerRepo& a, const ServerRepo& b)
{
    return a.mtime > b.mtime;
}

QRegExp makeFilterRegExp(const QString& text)
{
    return QRegExp(text.split(" ", QString::SkipEmptyParts).join(".*"),
                   Qt::CaseInsensitive);
}


} // namespace


RepoTreeModel::RepoTreeModel(QObject *parent)
    : QStandardItemModel(parent),
      tree_view_(NULL)
{
    initialize();

    refresh_local_timer_ = new QTimer(this);
    connect(refresh_local_timer_, SIGNAL(timeout()),
            this, SLOT(refreshLocalRepos()));

    refresh_local_timer_->start(kRefreshLocalReposInterval);
}
RepoTreeModel::~RepoTreeModel()
{
    // FIXME we have a memory leak here
    // however no perfect solution is provided so I left it alone
    if (item(kIndexOfVirtualReposCategory) != virtual_repos_category_)
        delete virtual_repos_category_;
}

void RepoTreeModel::initialize()
{
    recent_updated_category_ = new RepoCategoryItem(CAT_INDEX_RECENT_UPDATED, tr("Recently Updated"));
    my_repos_category_ = new RepoCategoryItem(CAT_INDEX_MY_REPOS, tr("My Libraries"));
    virtual_repos_category_ = new RepoCategoryItem(CAT_INDEX_VIRTUAL_REPOS, tr("Sub Libraries"));
    shared_repos_category_ = new RepoCategoryItem(CAT_INDEX_SHARED_REPOS, tr("Private Shares"));
    synced_repos_category_ = new RepoCategoryItem(CAT_INDEX_SYNCED_REPOS, tr("Synced Libraries"));

    appendRow(recent_updated_category_);
    appendRow(my_repos_category_);
    // appendRow(virtual_repos_category_);
    appendRow(shared_repos_category_);
    appendRow(synced_repos_category_);

    if (tree_view_) {
        tree_view_->restoreExpandedCategries();
    }
}

QModelIndex RepoTreeModel::proxiedIndexFromItem(const QStandardItem* item)
{
    QSortFilterProxyModel *proxy_model = (QSortFilterProxyModel *)tree_view_->model();
    return proxy_model->mapFromSource(indexFromItem(item));
}

void RepoTreeModel::clear()
{
    beginResetModel();
    QStandardItemModel::clear();
    initialize();
    endResetModel();
}

void RepoTreeModel::setRepos(const std::vector<ServerRepo>& repos)
{
    size_t i, n = repos.size();
    // removeReposDeletedOnServer(repos);

    clear();

    QHash<QString, ServerRepo> map;
    for (i = 0; i < n; i++) {
        const ServerRepo& repo = repos[i];
        if (repo.isPersonalRepo()) {
            if (repo.isVirtual()) {
                checkVirtualRepo(repo);
            } else {
                checkPersonalRepo(repo);
            }
        } else if (repo.isSharedRepo()) {
            checkSharedRepo(repo);
        } else {
            checkGroupRepo(repo);
        }

        if (repo.isSubfolder() || seafApplet->rpcClient()->hasLocalRepo(repo.id))
            checkSyncedRepo(repo);

        // we have a conflicting case, don't use group version if we can
        if (map.contains(repo.id) && repo.isGroupRepo())
            continue;
        map[repo.id] = repo;
    }

    QList<ServerRepo> list = map.values();
    // sort all repos by timestamp
    // use std::sort for qt containers will force additional copy.
    // anyway, we can use qt's alternative qSort for it
    qSort(list.begin(), list.end(), compareRepoByTimestamp);

    n = qMin(list.size(), kMaxRecentUpdatedRepos);
    for (i = 0; i < n; i++) {
        RepoItem *item = new RepoItem(list[i]);
        recent_updated_category_->appendRow(item);
    }
}

struct DeleteRepoData {
    QHash<QString, const ServerRepo*> map;
    QList<RepoItem*> itemsToDelete;
};

void RepoTreeModel::collectDeletedRepos(RepoItem *item, void *vdata)
{
    DeleteRepoData *data = (DeleteRepoData *)vdata;
    const ServerRepo* repo = data->map.value(item->repo().id);
    if (!repo || repo->type != item->repo().type) {
        data->itemsToDelete << item;
    }
}

void RepoTreeModel::removeReposDeletedOnServer(const std::vector<ServerRepo>& repos)
{
    int i, n;
    DeleteRepoData data;
    n = repos.size();
    for (i = 0; i < n; i++) {
        const ServerRepo& repo = repos[i];
        data.map.insert(repo.id, &repo);
    }

    forEachRepoItem(&RepoTreeModel::collectDeletedRepos, (void *)&data);

    QListIterator<RepoItem*> iter(data.itemsToDelete);
    while(iter.hasNext()) {
        RepoItem *item = iter.next();

        const ServerRepo& repo = item->repo();

        qDebug("remove repo %s(%s) from \"%s\"\n",
               toCStr(repo.name), toCStr(repo.id),
               toCStr(((RepoCategoryItem*)item->parent())->name()));

        item->parent()->removeRow(item->row());
    }
}


void RepoTreeModel::checkPersonalRepo(const ServerRepo& repo)
{
    int row, n = my_repos_category_->rowCount();
    for (row = 0; row < n; row++) {
        RepoItem *item = (RepoItem *)(my_repos_category_->child(row));
        if (item->repo().id == repo.id) {
            updateRepoItem(item, repo);
            return;
        }
    }

    // The repo is new
    RepoItem *item = new RepoItem(repo);
    my_repos_category_->appendRow(item);
}

void RepoTreeModel::checkVirtualRepo(const ServerRepo& repo)
{
    if (item(kIndexOfVirtualReposCategory) != virtual_repos_category_) {
        insertRow(kIndexOfVirtualReposCategory, virtual_repos_category_);
    }

    int row, n = virtual_repos_category_->rowCount();
    for (row = 0; row < n; row++) {
        RepoItem *item = (RepoItem *)(virtual_repos_category_->child(row));
        if (item->repo().id == repo.id) {
            updateRepoItem(item, repo);
            return;
        }
    }

    // The repo is new
    RepoItem *item = new RepoItem(repo);
    virtual_repos_category_->appendRow(item);
}

void RepoTreeModel::checkSharedRepo(const ServerRepo& repo)
{
    int row, n = shared_repos_category_->rowCount();
    for (row = 0; row < n; row++) {
        RepoItem *item = (RepoItem *)(shared_repos_category_->child(row));
        if (item->repo().id == repo.id) {
            updateRepoItem(item, repo);
            return;
        }
    }

    // the repo is a new one
    RepoItem *item = new RepoItem(repo);
    shared_repos_category_->appendRow(item);
}

void RepoTreeModel::checkGroupRepo(const ServerRepo& repo)
{
    QStandardItem *root = invisibleRootItem();
    RepoCategoryItem *group = NULL;

    int row, n = root->rowCount();

    for (row = 0; row < n; row ++) {
        RepoCategoryItem *item = (RepoCategoryItem *)(root->child(row));
        if (item->groupId() == repo.group_id) {
            group = item;
            break;
        }
    }
    if (!group) {
        if (repo.group_name == "Organization") {
            group = new RepoCategoryItem(CAT_INDEX_PUBLIC_REPOS, tr("Organization"), repo.group_id);
            // Insert pub repos after "recent updated", "my libraries", "shared libraries"
            insertRow(3, group);
        } else {
            group = new RepoCategoryItem(CAT_INDEX_GROUP_REPOS, repo.group_name, repo.group_id);
            appendRow(group);
        }
    }

    // Find the repo in this group
    n = group->rowCount();
    for (row = 0; row < n; row++) {
        RepoItem *item = (RepoItem *)(group->child(row));
        if (item->repo().id == repo.id) {
            updateRepoItem(item, repo);
            return;
        }
    }

    // Current repo not in this group yet
    RepoItem *item = new RepoItem(repo);
    group->appendRow(item);
}

void RepoTreeModel::checkSyncedRepo(const ServerRepo& repo)
{
    int row, n = synced_repos_category_->rowCount();
    for (row = 0; row < n; row++) {
        RepoItem *item = (RepoItem *)(synced_repos_category_->child(row));
        if (item->repo().id == repo.id) {
            updateRepoItem(item, repo);
            return;
        }
    }

    // The repo is new
    RepoItem *item = new RepoItem(repo);
    synced_repos_category_->appendRow(item);
}

void RepoTreeModel::updateRepoItem(RepoItem *item, const ServerRepo& repo)
{
    item->setRepo(repo);
}

void RepoTreeModel::forEachRepoItem(void (RepoTreeModel::*func)(RepoItem *, void *),
                                    void *data)
{
    int row;
    int n;
    QStandardItem *root = invisibleRootItem();
    n = root->rowCount();
    for (row = 0; row < n; row++) {
        RepoCategoryItem *category = (RepoCategoryItem *)root->child(row);
        int j, total;
        total = category->rowCount();
        for (j = 0; j < total; j++) {
            RepoItem *item = (RepoItem *)category->child(j);
            (this->*func)(item, data);
        }
    }
}

void RepoTreeModel::refreshLocalRepos()
{
    if (!seafApplet->mainWindow()->isVisible()) {
        return;
    }

    std::vector<CloneTask> tasks;
    seafApplet->rpcClient()->getCloneTasks(&tasks);

    forEachRepoItem(&RepoTreeModel::refreshRepoItem, (void*) &tasks);
}

void RepoTreeModel::refreshRepoItem(RepoItem *item, void *data)
{
    if (!tree_view_->isExpanded(proxiedIndexFromItem(item->parent()))) {
        return;
    }

    if (item->syncNowClicked()) {
        // Skip refresh repo item on which the user has clicked "sync now"
        item->setSyncNowClicked(false);
        return;
    }

    LocalRepo local_repo;
    seafApplet->rpcClient()->getLocalRepo(item->repo().id, &local_repo);
    if (local_repo != item->localRepo()) {
        item->setLocalRepo(local_repo);
        QModelIndex index = indexFromItem(item);
        emit dataChanged(index,index);
        emit repoStatusChanged(index);
        // printf("repo %s is changed\n", toCStr(item->repo().name));
    }

    item->setCloneTask();

    CloneTask clone_task;
    std::vector<CloneTask>* tasks = (std::vector<CloneTask>*)data;
    if (!local_repo.isValid()) {
        for (size_t i=0; i < tasks->size(); ++i) {
            clone_task = tasks->at(i);
            if (clone_task.repo_id == item->repo().id) {
                item->setCloneTask(clone_task);
                QModelIndex index = indexFromItem(item);
                emit dataChanged(index,index);
                emit repoStatusChanged(index);
            }
        }
    }
}

void RepoTreeModel::updateRepoItemAfterSyncNow(const QString& repo_id)
{
    QString id = repo_id;
    forEachRepoItem(&RepoTreeModel::updateRepoItemAfterSyncNow, (void*) &id);
}

void RepoTreeModel::updateRepoItemAfterSyncNow(RepoItem *item, void *data)
{
    QString repo_id = *(QString *)data;
    LocalRepo r = item->localRepo();
    if (r.isValid() && r.id == repo_id) {
        // We manually set the sync state of the repo to "SYNC_STATE_ING" to give
        // the user immediate feedback

        r.setSyncInfo("initializing");
        r.sync_state = LocalRepo::SYNC_STATE_ING;
        item->setLocalRepo(r);
        item->setSyncNowClicked(true);
    }
}

void RepoTreeModel::onFilterTextChanged(const QString& text)
{
    // Recalculate the matched repos count for each category
    QStandardItem *root = invisibleRootItem();
    int row, n;
    n = root->rowCount();
    QRegExp re = makeFilterRegExp(text);
    for (row = 0; row < n; row++) {
        RepoCategoryItem *category = (RepoCategoryItem *)root->child(row);
        int j, total, matched = 0;
        total = category->rowCount();
        for (j = 0; j < total; j++) {
            RepoItem *item = (RepoItem *)category->child(j);
            if (item->repo().name.contains(re)) {
                matched++;
            }
        }
        category->setMatchedReposCount(matched);
    }
}

RepoFilterProxyModel::RepoFilterProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent),
      has_filter_(false)
{
}

void RepoFilterProxyModel::setSourceModel(QAbstractItemModel *source_model)
{
    QSortFilterProxyModel::setSourceModel(source_model);
    RepoTreeModel *tree_model = (RepoTreeModel *)source_model;
    connect(tree_model, SIGNAL(repoStatusChanged(const QModelIndex&)),
            this, SLOT(onRepoStatusChanged(const QModelIndex&)));
}

void RepoFilterProxyModel::onRepoStatusChanged(const QModelIndex& source_index)
{
    QModelIndex index = mapFromSource(source_index);
    emit dataChanged(index, index);
}

bool RepoFilterProxyModel::filterAcceptsRow(int source_row,
                        const QModelIndex & source_parent) const
{
    RepoTreeModel *tree_model = (RepoTreeModel *)(sourceModel());
    QModelIndex index = tree_model->index(source_row, 0, source_parent);
    QStandardItem *item = tree_model->itemFromIndex(index);
    if (item->type() == REPO_CATEGORY_TYPE) {
        RepoCategoryItem *category = (RepoCategoryItem *)item;
        // We don't filter repo categories, only filter repos by name.
        return true;
    } else if (item->type() == REPO_ITEM_TYPE) {
        // Use default filtering (filter by item DisplayRole, i.e. repo name)
        bool match = QSortFilterProxyModel::filterAcceptsRow(source_row, source_parent);
        if (match) {
            RepoCategoryItem *category = (RepoCategoryItem *)(item->parent());
        }
        return match;
    }

    return false;
}

void RepoFilterProxyModel::setFilterText(const QString& text)
{
    has_filter_ = !text.isEmpty();
    invalidate();
    setFilterRegExp(makeFilterRegExp(text));
}

// void RepoFilterProxyModel::sort()
// {
// }

bool RepoFilterProxyModel::lessThan(const QModelIndex &left,
                                    const QModelIndex &right) const
{
    RepoTreeModel *tree_model = (RepoTreeModel *)(sourceModel());
    QStandardItem *item_l = tree_model->itemFromIndex(left);
    QStandardItem *item_r = tree_model->itemFromIndex(right);

    /**
     * When we have filter: sort category by matched repos count
     * When we have no filter: sort category by category index order
     *
     */
    if (item_l->type() == REPO_CATEGORY_TYPE) {
        // repo categories
        RepoCategoryItem *cl = (RepoCategoryItem *)item_l;
        RepoCategoryItem *cr = (RepoCategoryItem *)item_r;
        if (has_filter_) {
            // printf ("%s matched: %d, %s matched: %d\n",
            //         cl->name().toUtf8().data(), cl->matchedReposCount(),
            //         cr->name().toUtf8().data(), cr->matchedReposCount());
            return cl->matchedReposCount() > cr->matchedReposCount();
        } else {
            int cat_l = cl->categoryIndex();
            int cat_r = cr->categoryIndex();
            if (cat_l == cat_r) {
                return cl->name() < cr->name();
            } else {
                return cat_l < cat_r;
            }
        }
    } else {
        // repos
        RepoItem *cl = (RepoItem *)item_l;
        RepoItem *cr = (RepoItem *)item_r;
        return cl->repo().mtime > cr->repo().mtime;
    }

    return false;
}

Qt::ItemFlags RepoFilterProxyModel::flags(const QModelIndex& index) const
{
    Qt::ItemFlags flgs =  QSortFilterProxyModel::flags(index);
    return flgs & ~Qt::ItemIsEditable;
}
