#include <QtGui>

#include "rpc/local-repo.h"
#include "repo-item.h"

RepoItem::RepoItem(const LocalRepo& repo, QWidget *parent)
    : QWidget(parent),
      repo_(repo)
{
    setupUi(this);
    refresh();

    setFixedHeight(70);
}

void RepoItem::refresh()
{
    mRepoName->setText(repo_.name);
    mRepoIcon->setPixmap(QPixmap(":/images/repo.png"));
}
