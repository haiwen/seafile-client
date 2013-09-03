#include <QtGui>

#include "repos-view.h"
#include "repo-details-view.h"
#include "repos-tab.h"

ReposTab::ReposTab(QWidget *parent)
    : QWidget(parent)
{
    repos_view_ = new ReposView;
    repo_detail_view_ = new RepoDetailsView;
    QStackedLayout *layout = new QStackedLayout(this);

    layout->addWidget(repos_view_);
    layout->addWidget(repo_detail_view_);

    setLayout(layout);
}
