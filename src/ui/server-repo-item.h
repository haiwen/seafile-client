#ifndef SEAFILE_CLIENT_SERVER_REPO_ITEM_H
#define SEAFILE_CLIENT_SERVER_REPO_ITEM_H

#include <QWidget>
#include "ui_server-repo-item.h"
#include "api/server-repo.h"

class ServerRepoItem : public QWidget,
                       public Ui::ServerRepoItem
{
    Q_OBJECT

public:
    ServerRepoItem(const ServerRepo& repo, QWidget *parent=0);

    void refresh();
    void setRepo(const ServerRepo& repo) { repo_ = repo; }

    const ServerRepo& repo() const { return repo_; }

private:
    ServerRepo repo_;
};


#endif // SEAFILE_CLIENT_SERVER_REPO_ITEM_H
