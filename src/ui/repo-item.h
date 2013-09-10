#ifndef SEAFILE_CLIENT_REPO_ITEM_H
#define SEAFILE_CLIENT_REPO_ITEM_H

#include <QStandardItem>
#include "api/server-repo.h"
#include "rpc/local-repo.h"

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

    bool setRepo(const ServerRepo& repo);
    bool setLocalRepo(const LocalRepo& repo) { local_repo_ = repo; }

    virtual int type() const { return REPO_ITEM_TYPE; }

    const ServerRepo& repo() const { return repo_; }
    const LocalRepo& localRepo() const { return local_repo_; }

private:
    ServerRepo repo_;
    LocalRepo local_repo_;
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
