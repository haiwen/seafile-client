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

namespace {

const int kRefreshLocalReposInterval = 1000;
const int kMaxRecentUpdatedRepos = 10;

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
    recent_updated_category_ = new RepoCategoryItem(tr("Recent Updated"));
    my_repos_catetory_ = new RepoCategoryItem(tr("My Libraries"));
    shared_repos_catetory_ = new RepoCategoryItem(tr("Shared Libraries"));

    appendRow(recent_updated_category_);
    appendRow(my_repos_catetory_);
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

    for (i = 0; i < n; i++) {
        const ServerRepo& repo = repos[i];
        if (repo.isPersonalRepo()) {
            checkPersonalRepo(repo);
        } else if (repo.isSharedRepo()) {
            checkSharedRepo(repo);
        } else {
            checkGroupRepo(repo);
        }
    }

    std::vector<ServerRepo> repos_copy(repos);
    // sort all repso by timestamp
    std::sort(repos_copy.begin(), repos_copy.end(), compareRepoByTimestamp);
    // erase duplidates
    repos_copy.erase(std::unique(repos_copy.begin(), repos_copy.end(), isSameRepo), repos_copy.end());

    n = qMin((int)repos_copy.size(), kMaxRecentUpdatedRepos);
    for (i = 0; i < n; i++) {
        RepoItem *item = new RepoItem(repos_copy[i]);
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

    // First find for create the group
    // Starts from row 2 because the first two rows are "My Libraries" and "Shared Libraries"
    for (row = 2; row < n; row ++) {
        RepoCategoryItem *item = (RepoCategoryItem *)(root->child(row));
        if (item->groupId() == repo.group_id) {
            group = item;
            break;
        }
    }
    if (!group) {
        group = new RepoCategoryItem(repo.group_name, repo.group_id);
        appendRow(group);
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
    forEachRepoItem(&RepoTreeModel::refreshRepoItem, NULL);
}

void RepoTreeModel::refreshRepoItem(RepoItem *item, void *data)
{
    LocalRepo local_repo;
    seafApplet->rpcClient()->getLocalRepo(item->repo().id, &local_repo);
    if (local_repo != item->localRepo()) {
        item->setLocalRepo(local_repo);
        QModelIndex index = indexFromItem(item);
        emit dataChanged(index,index);
    }
}
