#ifndef SEAFILE_CLIENT_REPO_TREE_VIEW_H
#define SEAFILE_CLIENT_REPO_TREE_VIEW_H
#include <QTreeView>

class QAction;
class QContextMenuEvent;
class QModelIndex;
class QStandardItem;

class RepoItem;

class RepoTreeView : public QTreeView {
    Q_OBJECT
public:
    RepoTreeView(QWidget *parent=0);

protected:
    virtual void contextMenuEvent(QContextMenuEvent *event);

private slots:
    void downloadRepo();
    void showRepoDetail();
    void openLocalFolder();
    void onItemClicked(const QModelIndex& index);

private:
    QStandardItem* getRepoItem(const QModelIndex &index) const;

    void createContextMenu();
    void prepareContextMenu(const RepoItem *item);

    QMenu *context_menu_;
    QAction *download_action_;
    QAction *show_detail_action_;
    QAction *open_local_folder_action_;
};

#endif // SEAFILE_CLIENT_REPO_TREE_VIEW_H
