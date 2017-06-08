#include "seafile-applet.h"
#include "rpc/rpc-client.h"
#include "repo-item.h"

RepoItem::RepoItem(const ServerRepo& repo)
    : SeafileRepoBaseItem(),
      repo_(repo)
{
    setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    LocalRepo local_repo;
    seafApplet->rpcClient()->getLocalRepo(repo.id, &local_repo);
    setLocalRepo(local_repo);

    sync_now_clicked_ = false;
}

void RepoItem::setRepo(const ServerRepo& repo)
{
    repo_ = repo;
}

void RepoItem::setLocalRepo(const LocalRepo& repo)
{
    local_repo_ = repo;
}

bool RepoItem::repoDownloadable() const
{
    if (local_repo_.isValid()) {
        return false;
    }

    if (!clone_task_.isValid()) {
        return true;
    }

    QString state = clone_task_.state;
    if (state == "canceled" || state == "error" || state == "done") {
        return true;
    }

    return false;
}

QVariant RepoItem::data(int role) const
{
    if (role == Qt::DisplayRole) {
        return repo_.name;
    } else {
        return QVariant();
    }
}

RepoCategoryItem::RepoCategoryItem(int cat_index, const QString& name, int group_id)
    : SeafileRepoBaseItem(),
      cat_index_(cat_index),
      name_(name),
      group_id_(group_id),
      matched_repos_(-1)
{
    setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
}

QVariant RepoCategoryItem::data(int role) const
{
    if (role != Qt::DisplayRole) {
        return QVariant();
    }

    return name_;
}

int RepoCategoryItem::matchedReposCount() const
{
    if (isGroupsRoot()) {
        // This item is the groups root level.
        int i, n = rowCount(), sum = 0;
        for (i = 0; i < n; i++) {
            RepoCategoryItem *group = (RepoCategoryItem *)child(i);
            sum += group->matchedReposCount();
        }
        return sum;
    }

    // A normal group item.
    return matched_repos_ >= 0 ? matched_repos_ : rowCount();
}
