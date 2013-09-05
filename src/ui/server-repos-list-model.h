#ifndef SEAFILE_CLIENT_SERVER_REPO_LIST_MODEL_H
#define SEAFILE_CLIENT_SERVER_REPO_LIST_MODEL_H

#include <QAbstractListModel>

#include "api/server-repo.h"

class QModelIndex;

class ServerReposListModel : public QAbstractListModel {
    Q_OBJECT

public:
    explicit ServerReposListModel(QObject *parent=0);

	virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
	virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

    void setRepos(const std::vector<ServerRepo>& repos);

private:

    std::vector<ServerRepo> repos_;
};

#endif // SEAFILE_CLIENT_SERVER_REPO_LIST_MODEL_H
