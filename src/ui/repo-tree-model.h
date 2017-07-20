#ifndef SEAFILE_CLIENT_REPO_TREE_MODEL_H
#define SEAFILE_CLIENT_REPO_TREE_MODEL_H

#include <vector>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>
#include <QModelIndex>

class ServerRepo;
class RepoItem;
class RepoCategoryItem;
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
    /**
     * The default sorting order of repo categroies.
     */
    enum RepoCategoryIndex {
        CAT_INDEX_RECENT_UPDATED = 0,
        CAT_INDEX_MY_REPOS,
        CAT_INDEX_VIRTUAL_REPOS,
        CAT_INDEX_SHARED_REPOS,
        CAT_INDEX_PUBLIC_REPOS,
        CAT_INDEX_GROUP_REPOS,
        CAT_INDEX_SYNCED_REPOS,
    };

    RepoTreeModel(QObject *parent=0);
    ~RepoTreeModel();
    void setRepos(const std::vector<ServerRepo>& repos);

    void clear();

    void setTreeView(RepoTreeView *view) { tree_view_ = view; }
    RepoTreeView* treeView() { return tree_view_; }

    void updateRepoItemAfterSyncNow(const QString& repo_id);
    void onFilterTextChanged(const QString& text);
    int repo_category_height;

signals:
    void repoStatusChanged(const QModelIndex& index);

private slots:
    void refreshLocalRepos();

private:
    void checkPersonalRepo(const ServerRepo& repo);
    void checkVirtualRepo(const ServerRepo& repo);
    void checkSharedRepo(const ServerRepo& repo);
    void checkOrgRepo(const ServerRepo& repo);
    void checkGroupRepo(const ServerRepo& repo);
    void checkSyncedRepo(const ServerRepo& repo);
    void initialize();
    void updateRepoItem(RepoItem *item, const ServerRepo& repo);
    void refreshRepoItem(RepoItem *item, void *data);

    void forEachRepoItem(void (RepoTreeModel::*func)(RepoItem *, void *),
                         void *data,
                         QStandardItem *root = nullptr);

    void forEachCategoryItem(void (*func)(RepoCategoryItem*, void *),
                             void *data,
                             QStandardItem *root = nullptr);

    void removeReposDeletedOnServer(const std::vector<ServerRepo> &repos);

    void collectDeletedRepos(RepoItem *item, void *vdata);
    void updateRepoItemAfterSyncNow(RepoItem *item, void *data);
    QModelIndex proxiedIndexFromItem(const QStandardItem* item);

    RepoCategoryItem *recent_updated_category_;
    RepoCategoryItem *my_repos_category_;
    RepoCategoryItem *virtual_repos_category_;
    RepoCategoryItem *shared_repos_category_;
    RepoCategoryItem *org_repos_category_;
    RepoCategoryItem *groups_root_category_;
    RepoCategoryItem *synced_repos_category_;

    QTimer *refresh_local_timer_;

    RepoTreeView *tree_view_;
};

/**
 * This model is used to implement (only show repos filtered by user typed text)
 */
class RepoFilterProxyModel : public QSortFilterProxyModel {
    Q_OBJECT
public:
    RepoFilterProxyModel(QObject* parent=0);

    void setSourceModel(QAbstractItemModel *source_model);

    void setFilterText(const QString& text);
    bool filterAcceptsRow(int source_row,
                          const QModelIndex & source_parent) const;
    bool lessThan(const QModelIndex &left,
                  const QModelIndex &right) const;
    Qt::ItemFlags flags(const QModelIndex& index) const;

private slots:
    void onRepoStatusChanged(const QModelIndex& source_index);

private:
    bool has_filter_;
};

#endif // SEAFILE_CLIENT_REPO_TREE_MODEL_H
