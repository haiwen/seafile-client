#include <QtGui>

#include "repo-item.h"
#include "repos-view.h"

ReposView::ReposView(QWidget *parent) : QWidget(parent)
{
    setupUi(this);

    repos_list_ = new QWidget(this);
    repos_list_->setLayout(new QVBoxLayout);

    mScrollArea->setWidget(repos_list_);

    LocalRepo repo;
    repo.name = "repo001";
    repo.encrypted = true;

    addRepo(repo);
}

void ReposView::addRepo(const LocalRepo& repo)
{
    RepoItem *item = new RepoItem(repo);

    QVBoxLayout *layout = static_cast<QVBoxLayout*>(repos_list_->layout());

    layout->addWidget(item);
}
