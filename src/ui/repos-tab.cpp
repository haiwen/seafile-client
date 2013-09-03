#include <QtGui>

#include "repos-view.h"
#include "repo-details-view.h"
#include "repos-tab.h"

ReposTab::ReposTab(QWidget *parent)
    : QWidget(parent),
      repos_view_(new ReposView),
      repo_detail_view_(new RepoDetailsView)
{
}
