#include <QMap>

#include "api/server-repo.h"
#include "repo-item.h"
#include "repo-tree-model.h"


RepoTreeModel::RepoTreeModel(QObject *parent) : QStandardItemModel(parent)
{
    initialize();
}

void RepoTreeModel::initialize()
{
    my_repos_catetory_ = new RepoCategoryItem(tr("My Libraries"));
    shared_repos_catetory_ = new RepoCategoryItem(tr("Shared Libraries"));

    appendRow(my_repos_catetory_);
    appendRow(shared_repos_catetory_);
}

void RepoTreeModel::clear()
{
    QStandardItemModel::clear();
    initialize();
}

void RepoTreeModel::setRepos(const std::vector<ServerRepo>& repos)
{
    int i, n = repos.size();
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
    if (item->setRepo(repo)) {
        emit itemChanged(item);
    }
}
