#include <QModelIndex>

#include "server-repos-list-model.h"


ServerReposListModel::ServerReposListModel(QObject *parent): QAbstractListModel(parent)
{
}

void ServerReposListModel::setRepos(const std::vector<ServerRepo>& repos)
{
    if (repos.size() != repos_.size()) {
        beginResetModel();
        repos_ = repos;
        endResetModel();
        return;
    }
}

int ServerReposListModel::rowCount(const QModelIndex &parent) const
{
    return repos_.size();
}

QVariant ServerReposListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    ServerRepo repo = repos_[index.row()];
    switch(role) {
    case Qt::DisplayRole:
        return repo.name;
    case Qt::DecorationRole:
        return repo.getIcon();
    case RepoRole:
        return QVariant::fromValue(repo);
    }

    return QVariant();
}
