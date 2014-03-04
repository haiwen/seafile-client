#include "seafile-applet.h"
#include "rpc/rpc-client.h"
#include "repo-item.h"

RepoItem::RepoItem(const ServerRepo& repo)
    : repo_(repo)
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

RepoCategoryItem::RepoCategoryItem(const QString& name)
    : name_(name),
      group_id_(-1)
{
    setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
}

RepoCategoryItem::RepoCategoryItem(const QString& name, int group_id)
    : name_(name),
      group_id_(group_id)
{
    setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
}
