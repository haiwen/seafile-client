#include <vector>
#include <QtGui>
#include <QTimer>

#include "seafile-applet.h"
#include "server-repo-item.h"

#include "server-repos-view.h"

namespace {

const int kRefreshReposInterval = 30000;

}

ServerReposView::ServerReposView(QWidget *parent)
    : QWidget(parent),
      in_refresh_(false)
{
    setupUi(this);

    server_repos_list_ = new QWidget(this);
    server_repos_list_->setLayout(new QVBoxLayout);
    mScrollArea->setWidget(server_repos_list_);

    refresh_timer_ = new QTimer(this);
    connect(refresh_timer_, SIGNAL(timeout()), this, SLOT(refreshRepos()));
    connect(mRefreshButton, SIGNAL(clicked()),
            this, SLOT(refreshRepos()));
}

void ServerReposView::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    refreshRepos();
    refresh_timer_->start(kRefreshReposInterval);
}

void ServerReposView::hideEvent(QHideEvent *event) {
    QWidget::hideEvent(event);
    refresh_timer_->stop();
}

void ServerReposView::refreshRepos()
{
    if (in_refresh_) {
        return;
    }
    
    // SeafileRpcRequest *req = new SeafileRpcRequest();

    // connect(req, SIGNAL(listLocalReposSignal(const std::vector<LocalRepo>&, bool)),
    //         this, SLOT(refreshRepos(const std::vector<LocalRepo>&, bool)));

    // in_refresh_ = true;

    // req->listLocalRepos();
}

void ServerReposView::refreshRepos(const std::vector<ServerRepo>& repos, bool result)
{
    if (result) {
        int i, n = repos.size();
        for (i = 0; i < n; i++) {
            addRepo(repos[i]);
        }
    }

    in_refresh_ = false;
}

void ServerReposView::addRepo(const ServerRepo& repo)
{
    // if (repos_map_.contains(repo.id)) {
    //     RepoItem *item = repos_map_[repo.id];
    //     if (repo != item->repo()) {
    //         item->setRepo(repo);
    //         item->refresh();
    //     }
    //     return;
    // }

    // QVBoxLayout *layout = static_cast<QVBoxLayout*>(repos_list_->layout());
    // RepoItem *item = new RepoItem(repo);

    // repos_map_[repo.id] = item;
    // layout->addWidget(item);
}
