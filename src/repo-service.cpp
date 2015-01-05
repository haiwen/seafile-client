#include <QTimer>
#include <QDir>
#include <QDesktopServices>

#include "seafile-applet.h"
#include "rpc/rpc-client.h"
#include "rpc/local-repo.h"
#include "account-mgr.h"
#include "api/server-repo.h"
#include "api/requests.h"
#include "ui/main-window.h"
#include "ui/download-repo-dialog.h"
#include "utils/utils.h"

#include "repo-service.h"
#include "repo-service-helper.h"

namespace {

const int kRefreshReposInterval = 1000 * 60 * 5; // 5 min

} // namespace

SINGLETON_IMPL(RepoService)

RepoService::RepoService(QObject *parent)
    : QObject(parent)
{
    refresh_timer_ = new QTimer(this);
    connect(refresh_timer_, SIGNAL(timeout()), this, SLOT(refresh()));
    list_repo_req_ = NULL;
    in_refresh_ = false;
}

void RepoService::start()
{
    refresh_timer_->start(kRefreshReposInterval);
}

void RepoService::stop()
{
    refresh_timer_->stop();
}

void RepoService::refresh()
{
    if (in_refresh_) {
        return;
    }

    const std::vector<Account>& accounts = seafApplet->accountManager()->accounts();
    if (accounts.empty()) {
        in_refresh_ = false;
        return;
    }

    in_refresh_ = true;

    if (list_repo_req_) {
        delete list_repo_req_;
    }

    list_repo_req_ = new ListReposRequest(accounts[0]);

    connect(list_repo_req_, SIGNAL(success(const std::vector<ServerRepo>&)),
            this, SLOT(onRefreshSuccess(const std::vector<ServerRepo>&)));

    connect(list_repo_req_, SIGNAL(failed(const ApiError&)),
            this, SLOT(onRefreshFailed(const ApiError&)));
    list_repo_req_->send();
}

void RepoService::onRefreshSuccess(const std::vector<ServerRepo>& repos)
{
    in_refresh_ = false;

    server_repos_ = repos;

    emit refreshSuccess(repos);
}

void RepoService::onRefreshFailed(const ApiError& error)
{
    in_refresh_ = false;

    emit refreshFailed(error);
}

void RepoService::refresh(bool force)
{
    if (!force || !in_refresh_) {
        refresh();
        return;
    }

    // Abort the current request and send another
    in_refresh_ = false;
    refresh();
}

ServerRepo
RepoService::getRepo(const QString& repo_id) const
{
    int i, n = server_repos_.size();
    for (i = 0; i < n; i++) {
        ServerRepo repo = server_repos_[i];
        if (repo.id == repo_id) {
            return repo;
        }
    }

    return ServerRepo();
}

void RepoService::openLocalFile(const QString& repo_id,
                                const QString& path_in_repo,
                                QWidget *dialog_parent)
{
    LocalRepo r;

    seafApplet->rpcClient()->getLocalRepo(repo_id, &r);

    if (r.isValid()) {
        QString local_path = QDir(r.worktree).filePath(path_in_repo);

        FileDownloadHelper::openFile(local_path, false);
    } else {
        ServerRepo repo = getRepo(repo_id);
        if (!repo.isValid()) {
            return;
        }

        const QString path = "/" + path_in_repo;
        const Account account = seafApplet->accountManager()->currentAccount();
        FileDownloadHelper *helper =
          new FileDownloadHelper(account, repo, path, dialog_parent);
        helper->setParent(dialog_parent);
        helper->start();
    }
}
