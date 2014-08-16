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

#include "repo-service.h"

namespace {

const int kRefreshReposInterval = 1000 * 60 * 5; // 5 min

} // namespace

RepoService* RepoService::singleton_;

RepoService* RepoService::instance()
{
    if (singleton_ == NULL) {
        singleton_ = new RepoService;
    }

    return singleton_;
}

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
        QString path = QDir(r.worktree).filePath(path_in_repo);

        QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    } else {
        ServerRepo repo = getRepo(repo_id);
        if (!repo.isValid()) {
            return;
        }

        QString msg = tr("The library of this file is not synced yet. Do you want to sync it now?");
        if (seafApplet->yesOrNoBox(msg, NULL, true)) {
            Account account = seafApplet->accountManager()->currentAccount();
            if (account.isValid()) {
                QWidget *parent = dialog_parent ? dialog_parent : seafApplet->mainWindow();
                DownloadRepoDialog dialog(account, repo, parent);
                dialog.exec();
            }
        }
    }
}
