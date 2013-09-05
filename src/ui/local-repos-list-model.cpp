#include <QModelIndex>

#include "local-repos-list-model.h"


LocalReposListModel::LocalReposListModel(QObject *parent): QAbstractListModel(parent)
{
}

void LocalReposListModel::setRepos(const std::vector<LocalRepo>& repos)
{
    if (repos.size() != repos_.size()) {
        beginResetModel();
        repos_ = repos;
        endResetModel();
        return;
    }

    /**
       TODO:
       1. new RPC: get_repo_list_with_status
       2. check repo status here, and emit dataChanged() if repo status changed
       3. LocalRepo::getIcon returns an icon representing the repo sync status
    **/
}

int LocalReposListModel::rowCount(const QModelIndex &parent) const
{
    return repos_.size();
}

QVariant LocalReposListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    LocalRepo repo = repos_[index.row()];
    switch(role) {
    case Qt::DisplayRole:
        return repo.name;
    case Qt::DecorationRole:
        return repo.getIcon();
    }

    return QVariant();
}
