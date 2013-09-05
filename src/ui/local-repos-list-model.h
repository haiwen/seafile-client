#ifndef SEAFILE_CLIENT_LOCAL_REPO_LIST_MODEL_H
#define SEAFILE_CLIENT_LOCAL_REPO_LIST_MODEL_H

#include <QAbstractListModel>

#include "rpc/local-repo.h"

class QModelIndex;

class LocalReposListModel : public QAbstractListModel {
    Q_OBJECT

public:
    explicit LocalReposListModel(QObject *parent=0);

	virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
	virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

    void setRepos(const std::vector<LocalRepo>& repos);

private:

    std::vector<LocalRepo> repos_;
};

#endif // SEAFILE_CLIENT_LOCAL_REPO_LIST_MODEL_H
