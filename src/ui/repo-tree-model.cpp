#include <QTimer>
#include <QHash>
#include <QDebug>
#include <algorithm>            // std::sort

#include "api/server-repo.h"
#include "utils/utils.h"
#include "seafile-applet.h"
#include "main-window.h"
#include "rpc/rpc-client.h"
#include "repo-item.h"
#include "repo-tree-view.h"
#include "repo-tree-model.h"
#include "rpc/clone-task.h"

namespace {

const int kRefreshLocalReposInterval = 1000;
const int kMaxRecentUpdatedRepos = 10;
const int kIndexOfVirtualReposCategory = 2;

bool compareRepoByTimestamp(const ServerRepo& a, const ServerRepo& b)
{
    return a.mtime > b.mtime;
}

bool isSameRepo(const ServerRepo& a, const ServerRepo& b)
{
    return a.id == b.id;
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

void RepoTreeModel::initialize()
{
    recent_updated_category_ = new RepoCategoryItem(tr("Recently Updated"));
    my_repos_catetory_ = new RepoCategoryItem(tr("My Libraries"));
    virtual_repos_catetory_ = new RepoCategoryItem(tr("Sub Libraries"));
    shared_repos_catetory_ = new RepoCategoryItem(tr("Private Shares"));

    appendRow(recent_updated_category_);
    appendRow(my_repos_catetory_);
    // appendRow(virtual_repos_catetory_);
    appendRow(shared_repos_catetory_);

    if (tree_view_) {
        tree_view_->expand(indexFromItem(recent_updated_category_));
    }
}

void RepoTreeModel::clear()
{
    QStandardItemModel::clear();
    initialize();
}

void RepoTreeModel::setRepos(const std::vector<ServerRepo>& repos)
{
    int i, n = repos.size();
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

        map[repo.id] = repo;
    }

    QList<ServerRepo> list = map.values();
    // sort all repos by timestamp
    std::sort(list.begin(), list.end(), compareRepoByTimestamp);

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
    int row, n = my_repos_catetory_->rowCount();
    for (row = 0; row < n; row++) {
        RepoItem *item = (RepoItem *)(my_repos_catetory_->child(row));
        if (item->repo().id == repo.id) {
            updateRepoItem(item, repo);
            return;
        }
    }

    // The repo is new
    RepoItem *item = new RepoItem(repo);
    my_repos_catetory_->appendRow(item);
}

void RepoTreeModel::checkVirtualRepo(const ServerRepo& repo)
{
    if (item(kIndexOfVirtualReposCategory) != virtual_repos_catetory_) {
        insertRow(kIndexOfVirtualReposCategory, virtual_repos_catetory_);
    }

    int row, n = virtual_repos_catetory_->rowCount();
    for (row = 0; row < n; row++) {
        RepoItem *item = (RepoItem *)(virtual_repos_catetory_->child(row));
        if (item->repo().id == repo.id) {
            updateRepoItem(item, repo);
            return;
        }
    }

    // The repo is new
    RepoItem *item = new RepoItem(repo);
    virtual_repos_catetory_->appendRow(item);
}

void RepoTreeModel::checkSharedRepo(const ServerRepo& repo)
{
    int row, n = shared_repos_catetory_->rowCount();
    for (row = 0; row < n; row++) {
        RepoItem *item = (RepoItem *)(shared_repos_catetory_->child(row));
        if (item->repo().id == repo.id) {
            updateRepoItem(item, repo);
            return;
        }
    }

    // The repo is new
    RepoItem *item = new RepoItem(repo);
    shared_repos_catetory_->appendRow(item);
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
            group = new RepoCategoryItem(tr("Organization"), repo.group_id);
            // Insert pub repos after "recent updated", "my libraries", "shared libraries"
            insertRow(3, group);
        } else {
            group = new RepoCategoryItem(repo.group_name, repo.group_id);
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
    if (!tree_view_->isExpanded(indexFromItem(item->parent()))) {
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
        // qDebug("repo %s is changed\n", toCStr(item->repo().name));
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

        r.sync_state = LocalRepo::SYNC_STATE_ING;
        item->setLocalRepo(r);
        item->setSyncNowClicked(true);
    }
}
