#ifndef SEAFILE_CLIENT_REPO_ITEM_H
#define SEAFILE_CLIENT_REPO_ITEM_H

#include <QStandardItem>
#include "api/server-repo.h"
#include "rpc/local-repo.h"
#include "rpc/clone-task.h"

#define MY_REPOS "My Libraries"
#define SHARED_REPOS "Shared Libraries"

enum {
    REPO_ITEM_TYPE = QStandardItem::UserType,
    REPO_CATEGORY_TYPE
};

/**
 * Represent a repo
 */
class RepoItem : public QStandardItem {
public:
    RepoItem(const ServerRepo& repo);

    void setRepo(const ServerRepo& repo);
    void setLocalRepo(const LocalRepo& repo);

    virtual int type() const { return REPO_ITEM_TYPE; }

    const ServerRepo& repo() const { return repo_; }
    const LocalRepo& localRepo() const { return local_repo_; }

    /**
     * Although we don't use this in our custom delegate, we need to
     * implemented it for our proxy filter model.
     */
    QVariant data(int role=Qt::UserRole + 1) const;

    /**
     * Every time the item is painted, we record the metrics of each part of
     * the item on the screen. So later we the mouse click/hover the item, we
     * can decide which part is hovered, and to do corresponding actions.
     */
    struct Metrics {
        QRect icon_rect;
        QRect name_rect;
        QRect subtitle_rect;
        QRect status_icon_rect;
    };

    void setMetrics(const Metrics& metrics) const { metrics_ = metrics; }
    const Metrics& metrics() const { return metrics_; }

    void setCloneTask(const CloneTask& task=CloneTask()) { clone_task_ = task; }
    const CloneTask& cloneTask() const { return clone_task_; }

    bool repoDownloadable() const;

    void setSyncNowClicked(bool val) { sync_now_clicked_ = val; }
    bool syncNowClicked() const { return sync_now_clicked_; }

private:
    ServerRepo repo_;
    LocalRepo local_repo_;

    mutable Metrics metrics_;
    CloneTask clone_task_;

    bool sync_now_clicked_;
};

/**
 * Represent a repo category
 * E.g (My Repos, Shared repos, Group 1 repos, Group 2 repos ...)
 */
class RepoCategoryItem: public QStandardItem {
public:
    /**
     * Create a group category
     */
    RepoCategoryItem(int cat_index, const QString& name, int group_id=-1);

    virtual int type() const { return REPO_CATEGORY_TYPE; }

    // Accessors
    const QString& name() const { return name_; }

    bool isGroup() const { return group_id_ > 0; }

    int groupId() const { return group_id_; }

    /**
     * Although we don't use this in our custom delegate, we need to
     * implemented it for our proxy filter model.
     */
    QVariant data(int role=Qt::UserRole + 1) const;

    int categoryIndex() const { return cat_index_; }

    /**
     * Return the number of matched repos when the user types filter text
     */
    int matchedReposCount() const {
        return matched_repos_ >= 0
            ? matched_repos_
            : rowCount();
    }

    void resetMatchedRepoCount() { matched_repos_ = 0; };
    void setMatchedReposCount(int n) { matched_repos_ = n; };
    void increaseMatchedRepoCount() { matched_repos_++; };

private:
    QString name_;
    int group_id_;
    int matched_repos_;
    int cat_index_;
};

#endif // SEAFILE_CLIENT_REPO_ITEM_H
