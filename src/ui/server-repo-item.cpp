#include <QtGui>

#include "QtAwesome.h"
#include "utils/utils.h"
#include "api/server-repo.h"
#include "server-repo-item.h"

ServerRepoItem::ServerRepoItem(const ServerRepo& repo, QWidget *parent)
    : QWidget(parent),
      repo_(repo)
{
    setupUi(this);
    refresh();
}

void ServerRepoItem::refresh()
{
    mRepoName->setText(repo_.name);
    QPixmap pic(":/images/repo.svg");
    mRepoIcon->setPixmap(pic.scaled(48, 48,
                                    Qt::IgnoreAspectRatio,
                                    Qt::FastTransformation));

    mDownloadButton->setIcon(awesome->icon(icon_cloud_download));
}
