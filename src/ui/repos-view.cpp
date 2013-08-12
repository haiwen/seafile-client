#include <QtGui>

#include "repo-item.h"
#include "seafile-applet.h"
#include "rpc/rpc-client.h"
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
    RpcClient *rpc_client = seafApplet->rpc_client;
    if (!rpc_client->connected()) {
        return;
    }

    std::vector<LocalRepo> repos;
    if (rpc_client->listRepos(&repos) < 0) {
        return;
    }

    int i, n = repos.size();
    for (i = 0; i < n; i++) {
        addRepo(repos[i]);
    }
}

void ReposView::addRepo(const LocalRepo& repo)
{
    RepoItem *item = new RepoItem(repo);

    QVBoxLayout *layout = static_cast<QVBoxLayout*>(repos_list_->layout());

    layout->addWidget(item);
}
