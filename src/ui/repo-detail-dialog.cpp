#include <QtGlobal>

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
#include <QtWidgets>
#else
#include <QtGui>
#endif
#include <QDir>
#include <QTimer>

#include "utils/utils.h"
#include "account-mgr.h"
#include "seafile-applet.h"
#include "configurator.h"
#include "api/requests.h"
#include "rpc/rpc-client.h"
#include "rpc/clone-task.h"
#include "rpc/local-repo.h"

#include "repo-detail-dialog.h"

namespace {


const int kRefrshRepoStatusInterval = 1000; // 1s

} // namespace


RepoDetailDialog::RepoDetailDialog(const ServerRepo &repo, QWidget *parent)
    : QDialog(parent),
      repo_(repo)
{
    setupUi(this);
    setWindowTitle(tr("Library \"%1\"").arg(repo.name));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    mDesc->setText(repo.description);
    mTimeLabel->setText(translateCommitTime(repo.mtime));
    mOwnerLabel->setText(repo.owner);

    gint64 res = repo.size;
    QString filetyperes;

    if (res <= 1024) {
        filetyperes = "B";
    } else if (res > 1024 && res <= 1024*1024) {
        res = res / 1024;
        filetyperes = "KB";
    } else if (res > 1024*1024 && res <= 1024*1024*1024) {
        res = res / 1024 / 1024;
        filetyperes = "MB";
    } else if (res > 1024*1024*1024) {
        res = res / 1024 / 1024 / 1024;
        filetyperes = "GB";
    }
    mSizeLabel->setText(QString::number(res) + filetyperes);

    LocalRepo lrepo;
    seafApplet->rpcClient()->getLocalRepo(repo.id, &lrepo);
    if (lrepo.isValid()) {
        lpathLabel->setVisible(true);
        mLpathLabel->setVisible(true);
        mLpathLabel->setText(QDir::toNativeSeparators(lrepo.worktree));
        if (lrepo.sync_state == LocalRepo::SYNC_STATE_ERROR) {
            mStatus->setText(lrepo.sync_error_str);
        } else {
            mStatus->setText(lrepo.sync_state_str);
        }
    } else {
        mStatus->setText(tr("This library is not downloaded yet"));
        lpathLabel->setVisible(false);
        mLpathLabel->setVisible(false);
    }

    mRepoIcon->setPixmap(repo_.getPixmap());
    mRepoName->setText(repo_.name);
    #if defined(Q_OS_MAC)
    layout()->setContentsMargins(8, 9, 9, 4);
    layout()->setSpacing(5);
    #endif

    resize(sizeHint());
    updateRepoStatus();

    refresh_timer_ = new QTimer(this);
    connect(refresh_timer_, SIGNAL(timeout()), this, SLOT(updateRepoStatus()));
    refresh_timer_->start(kRefrshRepoStatusInterval);
}

void RepoDetailDialog::updateRepoStatus()
{
    LocalRepo r;
    QString text;
    seafApplet->rpcClient()->getLocalRepo(repo_.id, &r);
    if (r.isValid()) {
        if (r.sync_state == LocalRepo::SYNC_STATE_ERROR) {
            text = "<p style='color:red'>" + tr("Error: ") + r.sync_error_str + "</p>";
        } else {
            text = r.sync_state_str;
            if (r.sync_state == LocalRepo::SYNC_STATE_ING) {
                // add transfer rate and finished percent
                int rate, percent;
                if (seafApplet->rpcClient()->getRepoTransferInfo(repo_.id, &rate, &percent) == 0) {
                    text += ", " + QString::number(percent) + "%, " +  QString("%1 kB/s").arg(rate / 1024);
                }
            }
        }

    } else {
        std::vector<CloneTask> tasks;
        seafApplet->rpcClient()->getCloneTasks(&tasks);

        CloneTask task;

        if (!tasks.empty()) {
            for (size_t i = 0; i < tasks.size(); ++i) {
                CloneTask clone_task = tasks[i];
                if (clone_task.repo_id == repo_.id) {
                    task = clone_task;
                    break;
                }
            }
        }

        if (task.isValid() && task.isDisplayable()) {
            if (task.error_str.length() > 0) {
                text = task.error_str;
            } else {
                text = task.state_str;
            }
        } else {
            text = tr("This library is not downloaded yet");
        }
    }

    mStatus->setText(text);
}
