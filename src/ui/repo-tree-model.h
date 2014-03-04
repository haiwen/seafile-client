#ifndef SEAFILE_CLIENT_REPO_TREE_MODEL_H
#define SEAFILE_CLIENT_REPO_TREE_MODEL_H

#include <vector>
#include <QStandardItemModel>
class QModelIndex;

class ServerRepo;
class RepoCategoryItem;
class RepoItem;
class QTimer;
class RepoTreeView;

/**
 * Tree model for repos. There are two levels of items:
 *
 * The first level: (RepoCategory)
 *  - My Libraries
 *  - Shared Libraries
 *  - Group 1
 *  - Group 2
 *  - Group N
 *
 * The sencond level is the libraries belonging to the first level item (RepoItem)
 *
 *  - My Libraries
 *    - Notes
 *    - Musics
 *    - Logs
 */
class RepoTreeModel : public QStandardItemModel {
    Q_OBJECT

public:
    RepoTreeModel(QObject *parent=0);
    void setRepos(const std::vector<ServerRepo>& repos);

    void clear();

    void setTreeView(RepoTreeView *view) { tree_view_ = view; }
    RepoTreeView* treeView() { return tree_view_; }

    void updateRepoItemAfterSyncNow(const QString& repo_id);

private slots:
    void refreshLocalRepos();

private:
    void checkPersonalRepo(const ServerRepo& repo);
    void checkVirtualRepo(const ServerRepo& repo);
    void checkSharedRepo(const ServerRepo& repo);
    void checkGroupRepo(const ServerRepo& repo);
    void initialize();
    void updateRepoItem(RepoItem *item, const ServerRepo& repo);
    void refreshRepoItem(RepoItem *item, void *data);

    void forEachRepoItem(void (RepoTreeModel::*func)(RepoItem *, void *), void *data);

    void removeReposDeletedOnServer(const std::vector<ServerRepo>& repos);

    void collectDeletedRepos(RepoItem *item, void *vdata);
    void updateRepoItemAfterSyncNow(RepoItem *item, void *data);

    RepoCategoryItem *recent_updated_category_;
    RepoCategoryItem *my_repos_catetory_;
    RepoCategoryItem *virtual_repos_catetory_;
    RepoCategoryItem *shared_repos_catetory_;

    QTimer *refresh_local_timer_;

    RepoTreeView *tree_view_;

};

#endif // SEAFILE_CLIENT_REPO_TREE_MODEL_H
