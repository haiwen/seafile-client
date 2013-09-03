#include <vector>
#include <QtGui>
#include <QTimer>

#include "seafile-applet.h"
#include "rpc/rpc-request.h"
#include "rpc/local-repo.h"
#include "repo-item.h"

#include "repos-view.h"

namespace {

const int kRefreshReposInterval = 1000;

}

ReposView::ReposView(QWidget *parent)
    : QWidget(parent),
      in_refresh_(false)
{
    setupUi(this);

    repos_list_ = new QWidget(this);
    repos_list_->setLayout(new QVBoxLayout);
    mScrollArea->setWidget(repos_list_);

    refresh_timer_ = new QTimer(this);
    connect(refresh_timer_, SIGNAL(timeout()), this, SLOT(refreshRepos()));
}

void ReposView::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    refreshRepos();
    refresh_timer_->start(kRefreshReposInterval);
}

void ReposView::hideEvent(QHideEvent *event) {
    QWidget::hideEvent(event);
    refresh_timer_->stop();
}

void ReposView::refreshRepos()
{
    if (in_refresh_) {
        return;
    }
    
    SeafileRpcRequest *req = new SeafileRpcRequest();

    connect(req, SIGNAL(listLocalReposSignal(const std::vector<LocalRepo>&, bool)),
            this, SLOT(refreshRepos(const std::vector<LocalRepo>&, bool)));

    in_refresh_ = true;

    req->listLocalRepos();
}

void ReposView::refreshRepos(const std::vector<LocalRepo>& repos, bool result)
{
    if (result) {
        int i, n = repos.size();
        for (i = 0; i < n; i++) {
            addRepo(repos[i]);
        }
    }

    in_refresh_ = false;
}

void ReposView::addRepo(const LocalRepo& repo)
{
    if (repos_map_.contains(repo.id)) {
        return;
    }

    QVBoxLayout *layout = static_cast<QVBoxLayout*>(repos_list_->layout());
    RepoItem *item = new RepoItem(repo);

    repos_map_[repo.id] = item;
    layout->addWidget(item);
}
