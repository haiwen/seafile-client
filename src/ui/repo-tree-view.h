#ifndef SEAFILE_CLIENT_REPO_TREE_VIEW_H
#define SEAFILE_CLIENT_REPO_TREE_VIEW_H
#include <QTreeView>

class QModelIndex;
class RepoItem;

class RepoTreeView : public QTreeView {
    Q_OBJECT
public:
    RepoTreeView(QWidget *parent=0);

protected:
    void drawBranches(QPainter *painter,
                      const QRect& rect,
                      const QModelIndex & index) const;

    RepoItem* getRepoItem(const QModelIndex &index) const;
};

#endif // SEAFILE_CLIENT_REPO_TREE_VIEW_H
