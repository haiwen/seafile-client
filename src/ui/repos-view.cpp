#include <vector>
#include <QtGui>

#include "seafile-applet.h"
#include "rpc/rpc-request.h"
#include "rpc/local-repo.h"
#include "repo-item.h"

#include "repos-view.h"

ReposView::ReposView(QWidget *parent) : QWidget(parent)
{
    setupUi(this);

    repos_list_ = new QWidget(this);
    repos_list_->setLayout(new QVBoxLayout);

    mScrollArea->setWidget(repos_list_);
}

void ReposView::updateRepos()
{
    SeafileRpcRequest *req = new SeafileRpcRequest();

    connect(req, SIGNAL(listLocalReposSignal(const std::vector<LocalRepo>&, bool)),
            this, SLOT(updateRepos(const std::vector<LocalRepo>&, bool)));

    req->listLocalRepos();
}

void ReposView::updateRepos(const std::vector<LocalRepo>& repos, bool result)
{
    if (result) {
        int i, n = repos.size();
        for (i = 0; i < n; i++) {
            addRepo(repos[i]);
        }
    }
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
