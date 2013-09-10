#ifndef SEAFILE_CLIENT_REPO_TREE_VIEW_H
#define SEAFILE_CLIENT_REPO_TREE_VIEW_H
#include <QTreeView>

class QAction;
class QContextMenuEvent;
class QModelIndex;

class RepoItem;
class ServerRepo;

class RepoTreeView : public QTreeView {
    Q_OBJECT
public:
    RepoTreeView(QWidget *parent=0);

protected:
    virtual void contextMenuEvent(QContextMenuEvent *event);
    void drawBranches(QPainter *painter,
                      const QRect& rect,
                      const QModelIndex & index) const;

private slots:
    void downloadRepo();
    void showRepoDetail();
    void openLocalFolder();

private:
    RepoItem* getRepoItem(const QModelIndex &index) const;

    void createContextMenu();
    void prepareContextMenu(const ServerRepo& repo);
    QChar getSyncStatusIcon(const ServerRepo& repo) const;

    QMenu *context_menu_;
    QAction *download_action_;
    QAction *show_detail_action_;
    QAction *open_local_folder_action_;
};

#endif // SEAFILE_CLIENT_REPO_TREE_VIEW_H
