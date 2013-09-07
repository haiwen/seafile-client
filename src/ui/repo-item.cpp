#include "repo-item.h"

RepoItem::RepoItem(const ServerRepo& repo)
    : repo_(repo)
{
    setData(repo.getIcon(), Qt::DecorationRole);
    setData(repo.name, Qt::DisplayRole);

    setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
}

void RepoItem::setRepo(const ServerRepo& repo) {
    repo_ = repo;
    // TODO: emit changed signal if repo info changed
}

RepoCategoryItem::RepoCategoryItem(const QString& name)
    : name_(name),
      group_id_(-1)
{
    setData(name, Qt::DisplayRole);
    setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
}

RepoCategoryItem::RepoCategoryItem(const QString& name, int group_id)
    : name_(name),
      group_id_(group_id)
{
    setData(name, Qt::DisplayRole);
    setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
}
