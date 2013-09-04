#ifndef SEAFILE_CLIENT_SERVER_REPOS_VIEW_H
#define SEAFILE_CLIENT_SERVER_REPOS_VIEW_H

#include <QWidget>
#include <QHash>
#include "ui_server-repos-view.h"

class QTimer;
class QShowEvent;
class QHideEvent;

class ServerRepo;
class ServerRepoItem;

class ServerReposView : public QWidget,
                        public Ui::ServerReposView
{
    Q_OBJECT

public:
    ServerReposView(QWidget *parent=0);

protected:
    void showEvent(QShowEvent *event);
    void hideEvent(QHideEvent *event);

private slots:
    void refreshRepos(const std::vector<ServerRepo>& repos, bool result);
    void refreshRepos();

private:
    Q_DISABLE_COPY(ServerReposView)

    void addRepo(const ServerRepo& repo);

    QWidget *server_repos_list_;
    QHash<QString, ServerRepoItem*> repos_map_;

    QTimer *refresh_timer_;
    bool in_refresh_;
};


#endif  // SEAFILE_CLIENT_SERVER_REPOS_VIEW_H
