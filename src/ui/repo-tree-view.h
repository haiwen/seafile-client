#ifndef SEAFILE_CLIENT_REPO_TREE_VIEW_H
#define SEAFILE_CLIENT_REPO_TREE_VIEW_H
#include <QTreeView>

class QAction;
class QContextMenuEvent;
class QEvent;
class QModelIndex;
class QStandardItem;

class RepoItem;
class RepoCategoryItem;
class CloudView;

class RepoTreeView : public QTreeView {
    Q_OBJECT
public:
    RepoTreeView(CloudView *view, QWidget *parent=0);

protected:
    virtual void contextMenuEvent(QContextMenuEvent *event);
    virtual bool viewportEvent(QEvent *event);

private slots:
    void downloadRepo();
    void showRepoDetail();
    void openLocalFolder();
    void viewRepoOnWeb();
    void onItemClicked(const QModelIndex& index);

private:
    QStandardItem* getRepoItem(const QModelIndex &index) const;

    void createContextMenu();
    void prepareContextMenu(const RepoItem *item);

    void showRepoItemToolTip(const RepoItem *item,
                             const QPoint& pos,
                             const QRect& rect);

    void showRepoCategoryItemToolTip(const RepoCategoryItem *item,
                                     const QPoint& pos,
                                     const QRect& rect);

    QMenu *context_menu_;
    QAction *download_action_;
    QAction *show_detail_action_;
    QAction *open_local_folder_action_;
    QAction *view_on_web_action_;

    CloudView *cloud_view_;
};

#endif // SEAFILE_CLIENT_REPO_TREE_VIEW_H
