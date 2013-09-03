#include <QtGui>

#include "rpc/local-repo.h"
#include "QtAwesome.h"
#include "utils/utils.h"
#include "repo-item.h"

RepoItem::RepoItem(const LocalRepo& repo, QWidget *parent)
    : QWidget(parent),
      repo_(repo)
{
    setupUi(this);
    refresh();

    setFixedHeight(70);

    connect(mOpenFolderButton, SIGNAL(clicked()), 
            this, SLOT(openRepoFolder()));
}

void RepoItem::refresh()
{
    mRepoName->setText(repo_.name);
    QPixmap pic(":/images/repo.svg");
    mRepoIcon->setPixmap(pic.scaled(48, 48,
                                    Qt::IgnoreAspectRatio,
                                    Qt::FastTransformation));

    mSyncStatusButton->setIcon(awesome->icon(icon_ok));
    mOpenFolderButton->setIcon(awesome->icon(icon_folder_open_alt));
}

void RepoItem::openRepoFolder()
{
    open_dir(repo_.worktree);
}
