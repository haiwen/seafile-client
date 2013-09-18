#include <QtGui>

#include "account-mgr.h"
#include "seafile-applet.h"
#include "configurator.h"
#include "api/requests.h"
#include "rpc/rpc-client.h"
#include "repo-detail-dialog.h"


RepoDetailDialog::RepoDetailDialog(const ServerRepo &repo, QWidget *parent)
    : QDialog(parent),
      repo_(repo)
{
    setupUi(this);
    setWindowTitle(tr("Library \"%1\"").arg(repo.name));

    mDesc->setText(repo.description);
    const QDateTime dt = QDateTime::fromTime_t(repo.mtime);
    mTimeLabel->setText(dt.toString(Qt::TextDate));        
    mOwnerLabel->setText(repo.owner);


    gint64 res = repo.size;
    QString filetyperes;

    if (res <= 1024) {
        res = res;
        filetyperes = "B";
    } else if (res > 1024 && res <= 1024*1024) {
        res = res / 1024;
        filetyperes = "KiB";
    } else if (res > 1024*1024 && res <= 1024*1024*1024) { 
        res = res / 1024 / 1024; 
        filetyperes = "MiB";
    } else if (res > 1024*1024*1024) { 
        res = res / 1024 / 1024 / 1024; 
        filetyperes = "GiB";
    }
    mSizeLabel->setText(QString::number(res) + filetyperes);

    if (!repo.isGroupRepo()) {
        mGroupLabel->setVisible(false);
        groupLabel->setVisible(false);
        mGroupLabel->setText(repo.group_name);
    } else {
        mGroupLabel->setVisible(true);
        groupLabel->setVisible(true);
    }
    progressBar->setVisible(false);
}

RepoDetailDialog::~RepoDetailDialog()
{

}

