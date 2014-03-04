#ifndef SEAFILE_CLIENT_REPO_ITEM_H
#define SEAFILE_CLIENT_REPO_ITEM_H

#include <QStandardItem>
#include "api/server-repo.h"
#include "rpc/local-repo.h"
#include "rpc/clone-task.h"

#define MY_REPOS "My Libraries"
#define SHARED_REPOS "Shared Libraries"

enum {
    REPO_ITEM_TYPE,
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
     * Create a non-group category
     */
    RepoCategoryItem(const QString& name);

    /**
     * Create a group category
     */
    RepoCategoryItem(const QString& name, int group_id);

    virtual int type() const { return REPO_CATEGORY_TYPE; }

    // Accessors
    const QString& name() const { return name_; }

    bool isGroup() const { return group_id_ > 0; }

    int groupId() const { return group_id_; }

private:
    QString name_;
    int group_id_;
};

#endif // SEAFILE_CLIENT_REPO_ITEM_H
