#include "finder-sync/finder-sync-host.h"

#include <vector>
#include <mutex>
#include <memory>

#include <QTimer>
#include <QDir>
#include <QFileInfo>

#include "account.h"
#include "account-mgr.h"
#include "settings-mgr.h"
#include "seafile-applet.h"
#include "rpc/local-repo.h"
#include "rpc/rpc-client.h"
#include "filebrowser/file-browser-requests.h"
#include "filebrowser/sharedlink-dialog.h"

static std::mutex watch_set_mutex_;
static std::vector<LocalRepo> watch_set_;
static std::unique_ptr<GetSharedLinkRequest> get_shared_link_req_;
static constexpr int kUpdateWatchSetInterval = 5000;

FinderSyncHost::FinderSyncHost()
  : timer_(new QTimer(this))
{
    timer_->setSingleShot(true);
    timer_->start(kUpdateWatchSetInterval);
    connect(timer_, SIGNAL(timeout()), this, SLOT(updateWatchSet()));
}

FinderSyncHost::~FinderSyncHost() {
    get_shared_link_req_.reset();
    timer_->stop();
}

size_t FinderSyncHost::getWatchSet(watch_dir_t *front, size_t max_size) {
    std::lock_guard<std::mutex> watch_set_lock(watch_set_mutex_);

    size_t count = (watch_set_.size() > max_size) ? max_size : watch_set_.size();
    for (size_t i = 0; i != count; ++i, ++front) {
        strncpy(front->body, watch_set_[i].worktree.toUtf8().data(),
                kPathMaxSize);
        front->status = watch_set_[i].sync_state;
    }
    return count;
}

void FinderSyncHost::updateWatchSet() {
    std::unique_lock<std::mutex> watch_set_lock(watch_set_mutex_);

    SeafileRpcClient *rpc = seafApplet->rpcClient();

    // update watch_set_
    watch_set_.clear();
    if (rpc->listLocalRepos(&watch_set_))
        qWarning("[FinderSync] update watch set failed");
    if (seafApplet->settingsManager()->autoSync()) {
        for (LocalRepo &repo : watch_set_)
            rpc->getSyncStatus(repo);
    } else {
        for (LocalRepo &repo : watch_set_)
            repo.sync_state = LocalRepo::SYNC_STATE_DISABLED;
    }
    watch_set_lock.unlock();

    timer_->start(kUpdateWatchSetInterval);
}

void FinderSyncHost::doShareLink(const QString &path) {
    QString repo_id;
    QString worktree_path;
    {
        std::unique_lock<std::mutex> watch_set_lock(watch_set_mutex_);
        for (const LocalRepo &repo : watch_set_)
            if (path.startsWith(repo.worktree)) {
                repo_id = repo.id;
                worktree_path = repo.worktree;
                break;
            }
    }
    QDir worktree_dir(worktree_path);
    QString path_in_repo = worktree_dir.relativeFilePath(path);
    // we have a empty path_in_repo representing the root of the directory,
    // and we are okay!

    if (repo_id.isEmpty() || path_in_repo.startsWith(".")) {
        qWarning("[FinderSync] invalid path %s", path.toUtf8().data());
        return;
    }

    const Account account = seafApplet->accountManager()->getAccountByRepo(repo_id);
    if (!account.isValid()) {
        qWarning("[FinderSync] invalid repo_id %s", repo_id.toUtf8().data());
        return;
    }

    get_shared_link_req_.reset(new GetSharedLinkRequest(
        account, repo_id, QString("/").append(path_in_repo),
        QFileInfo(path).isFile()));

    connect(get_shared_link_req_.get(), SIGNAL(success(const QString&)),
            this, SLOT(onShareLinkGenerated(const QString&)));

    get_shared_link_req_->send();

}

void FinderSyncHost::onShareLinkGenerated(const QString& link)
{
    SharedLinkDialog *dialog = new SharedLinkDialog(link, NULL);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
    dialog->raise();
    dialog->activateWindow();
}
