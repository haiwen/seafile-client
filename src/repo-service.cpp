#include <unistd.h>
#include <algorithm>

#include <sqlite3.h>

#include <QTimer>
#include <QDir>
#include <QSet>
#include <QDesktopServices>
#include <QThreadPool>

#include "configurator.h"
#include "seafile-applet.h"
#include "rpc/rpc-client.h"
#include "rpc/local-repo.h"
#include "rpc/clone-task.h"
#include "account-mgr.h"
#include "api/server-repo.h"
#include "api/requests.h"
#include "ui/main-window.h"
#include "utils/utils.h"
#include "utils/file-utils.h"
#include "utils/uninstall-helpers.h"

#include "filebrowser/file-browser-manager.h"
#include "filebrowser/data-cache.h"
#include "filebrowser/data-mgr.h"

#include "repo-service.h"
#include "repo-service-helper.h"

namespace {

class SyncedSubfolder {
public:
    SyncedSubfolder() {}
    SyncedSubfolder(const QString &repo_id, const QString &parent_repo_id,
                    const QString &parent_path)
        : repo_id_(repo_id), parent_repo_id_(parent_repo_id),
          parent_path_(parent_path) {}
    SyncedSubfolder(const ServerRepo &repo, const ServerRepo &parent_repo,
                    const QString &parent_path)
        : repo_id_(repo.id), parent_repo_id_(parent_repo.id),
          parent_path_(parent_path) {}
    SyncedSubfolder(const SyncedSubfolder &rhs)
        : repo_id_(rhs.repo_id_), parent_repo_id_(rhs.parent_repo_id_),
          parent_path_(rhs.parent_path_) {}

    SyncedSubfolder& operator=(const SyncedSubfolder &rhs)
    {
        repo_id_ = rhs.repo_id_;
        parent_repo_id_ = rhs.parent_repo_id_;
        parent_path_ = rhs.parent_path_;
        return *this;
    }

    const QString& repoId() const { return repo_id_; }
    const QString& parentRepoId() const { return parent_repo_id_; }
    const QString& parentPath() const { return parent_path_; }

    bool isValid() const { return !repo_id_.isEmpty(); }

private:
    QString repo_id_;
    QString parent_repo_id_;
    QString parent_path_;
};

const int kRefreshReposInterval = 1000 * 60 * 5; // 5 min

bool loadSyncedFolderCB(sqlite3_stmt *stmt, void *data)
{
    std::vector<SyncedSubfolder> *synced_subfolders = static_cast<std::vector<SyncedSubfolder> *>(data);
    const char *repo_id = (const char *)sqlite3_column_text (stmt, 0);
    const char *parent_repo_id = (const char *)sqlite3_column_text (stmt, 1);
    const char *parent_path = (const char *)sqlite3_column_text (stmt, 2);
    synced_subfolders->push_back(SyncedSubfolder(repo_id, parent_repo_id, parent_path));

    return true;
}

// TODO: use lambda to replace this helper class if we are switching to C++11
template<class T>
class RepohasRepoID {
public:
    RepohasRepoID(const QString &repo_id) : repo_id_(repo_id) {}
    bool operator()(const T &repo) {
        return repo.id == repo_id_;
    }

private:
    const QString& repo_id_;
};

class SyncedSubfolderhasRepoID {
public:
    SyncedSubfolderhasRepoID(const QString &repo_id) : repo_id_(repo_id) {}
    bool operator()(const SyncedSubfolder &subfolder) {
        return subfolder.repoId() == repo_id_;
    }

private:
    const QString& repo_id_;
};

// don't rely on this,
// it is used for get repo request only
std::vector<SyncedSubfolder> synced_subfolders_;

} // namespace

SINGLETON_IMPL(RepoService)

RepoService::RepoService(QObject *parent)
    : QObject(parent), synced_subfolder_db_(NULL)
{
    refresh_timer_ = new QTimer(this);
    connect(refresh_timer_, SIGNAL(timeout()), this, SLOT(refresh()));
    list_repo_req_ = NULL;
    in_refresh_ = false;
    wipe_in_progress_ = false;
}

RepoService::~RepoService()
{
    if (synced_subfolder_db_)
        sqlite3_close(synced_subfolder_db_);
}

void RepoService::start()
{
    const char *errmsg;

    do {
        QString db_path = QDir(seafApplet->configurator()->seafileDir()).filePath("accounts.db");
        if (sqlite3_open (db_path.toUtf8().data(), &synced_subfolder_db_)) {
            errmsg = sqlite3_errmsg (synced_subfolder_db_);
            qWarning("failed to open synced subfolder database %s: %s",
                    db_path.toUtf8().data(), errmsg ? errmsg : "no error given");

            sqlite3_close(synced_subfolder_db_);
            synced_subfolder_db_ = NULL;
            break;
        }

        // enabling foreign keys, it must be done manually from each connection
        // and this feature is only supported from sqlite 3.6.19
        const char *sql = "PRAGMA foreign_keys=ON;";
        if (sqlite_query_exec (synced_subfolder_db_, sql) < 0) {
            qWarning("sqlite version is too low to support foreign key feature\n");
            qWarning("feature synced_sub_folder is disabled\n");
            sqlite3_close(synced_subfolder_db_);
            synced_subfolder_db_ = NULL;
            break;
        }

        // create SyncedSubfolder table
        sql = "CREATE TABLE IF NOT EXISTS SyncedSubfolder ("
            "repo_id TEXT PRIMARY KEY, parent_repo_id TEXT NOT NULL, "
            "url VARCHAR(24), username VARCHAR(15), "
            "parent_path TEXT NOT NULL, "
            "FOREIGN KEY(url, username) REFERENCES Accounts(url, username) "
            "ON DELETE CASCADE ON UPDATE CASCADE )";
        if (sqlite_query_exec (synced_subfolder_db_, sql) < 0) {
            qWarning("failed to create synced subfolder table\n");
            sqlite3_close(synced_subfolder_db_);
            synced_subfolder_db_ = NULL;
        }
    } while (0);

    refresh_timer_->start(kRefreshReposInterval);
}

void RepoService::stop()
{
    refresh_timer_->stop();
}

void RepoService::refreshLocalRepoList() {
    // local_repos_.clear(); is called intenally
    if (seafApplet->rpcClient()->listLocalRepos(&local_repos_) < 0) {
        qWarning("unable to refresh local repos\n");
    }
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

    const Account *account = &accounts.front();

    if (!account->isValid())
        return;

    in_refresh_ = true;

    if (!get_repo_reqs_.empty()) {
        for (std::list<GetRepoRequest*>::iterator pos = get_repo_reqs_.begin(); pos != get_repo_reqs_.end(); ++pos)
          delete (*pos);
        get_repo_reqs_.clear();
    }

    refreshLocalRepoList();

    synced_subfolders_.clear();
    if (synced_subfolder_db_) {

        char *zql = sqlite3_mprintf("SELECT repo_id, parent_repo_id, parent_path FROM SyncedSubfolder "
                                    "WHERE url = %Q AND username = %Q",
                                    account->serverUrl.toEncoded().data(),
                                    account->username.toUtf8().data());;
        sqlite_foreach_selected_row (synced_subfolder_db_, zql,
                                     loadSyncedFolderCB, &synced_subfolders_);
        sqlite3_free(zql);

        std::vector<CloneTask> tasks;
        seafApplet->rpcClient()->getCloneTasks(&tasks);

        QSet<QString> local_repos;
        for (size_t i = 0; i < tasks.size(); ++i) {
            local_repos.insert(tasks[i].repo_id);
        }
        for (size_t i = 0; i < local_repos_.size(); ++i) {
            local_repos.insert(local_repos_[i].id);
        }

        // If the synced virtual repo no longer exists locally, remove it from
        // applet database.
        for (size_t i = 0; i < synced_subfolders_.size(); ++i) {
            if (!local_repos.contains(synced_subfolders_[i].repoId())) {
              removeSyncedSubfolder(synced_subfolders_[i].repoId());
            }
        }
    }

    if (list_repo_req_) {
        list_repo_req_->deleteLater();
    }

    list_repo_req_ = new ListReposRequest(*account);

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

    // if we have local repo missed in server_repos
    // start a GetRepoRequest for it
    for (size_t i = 0; i < synced_subfolders_.size(); ++i) {
        bool found = false;
        for (size_t j = 0; j < server_repos_.size(); ++j) {
            if (synced_subfolders_[i].repoId() == server_repos_[j].id) {
                found = true;
                server_repos_[j].parent_repo_id = synced_subfolders_[i].parentRepoId();
                server_repos_[j].parent_path = synced_subfolders_[i].parentPath();
                break;
            }
        }
        // create GetRepoRequest
        if (!found)
            startGetRequestFor(synced_subfolders_[i].repoId());
    }

    if (get_repo_reqs_.empty())
        emit refreshSuccess(repos);
}

void RepoService::onRefreshFailed(const ApiError& error)
{
    in_refresh_ = false;

    if (list_repo_req_->reply()->hasRawHeader("X-Seafile-Wiped")) {
        qWarning ("current device is marked to be remote wiped\n");
        // TODO: Remote wipe should be managed in a separate module, not here.
        if (!wipe_in_progress_) {
            wipe_in_progress_ = true;
            wipeLocalFiles();
        }
        return;
    }

    // for the new version of seafile server
    // we may have a 401 response whenever invalid token is used.
    // see more: https://github.com/haiwen/seahub/commit/94dcfe338a52304f5895914ac59540b6176c679e
    // but we only handle this error here to avoid complicate code since it is
    // general enough.
    // TODO move this handling to all possible cases
    if (error.type() == ApiError::HTTP_ERROR && error.httpErrorCode() == 401) {
        seafApplet->accountManager()->invalidateCurrentLogin();
        return;
    }

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
    size_t n = server_repos_.size();
    for (size_t i = 0; i < n; i++) {
        const ServerRepo &repo = server_repos_[i];
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
    if (path_in_repo.endsWith("/")) {
        openFolder(repo_id, path_in_repo.left(path_in_repo.size() - 1));
        return;
    }
    qDebug("trying to open file %s in library %s", path_in_repo.toUtf8().data(), repo_id.toUtf8().data());

    LocalRepo r;

    seafApplet->rpcClient()->getLocalRepo(repo_id, &r);

    if (r.isValid()) {
        QString local_path = ::pathJoin(r.worktree, path_in_repo);

        FileDownloadHelper::openFile(local_path, false);
    } else {
        ServerRepo repo = getRepo(repo_id);
        if (!repo.isValid()) {
            QString msg = tr("Unable to open file \"%1\" from nonexistent library \"%2\"").arg(path_in_repo).arg(repo_id);
            seafApplet->warningBox(msg);
            return;
        }

        const QString path = "/" + path_in_repo;
        //TODO select the correct account
        const Account account = seafApplet->accountManager()->currentAccount();
        if (!account.isValid()) {
            qWarning("no valid account found");
            return;
        }
        FileDownloadHelper *helper =
          new FileDownloadHelper(account, repo, path, dialog_parent);
        helper->setParent(dialog_parent);
        helper->start();
    }
}

void RepoService::openFolder(const QString &repo_id,
                             const QString &path_in_repo)
{
    qDebug("trying to open folder %s in library %s", path_in_repo.toUtf8().data(), repo_id.toUtf8().data());
    LocalRepo r;

    seafApplet->rpcClient()->getLocalRepo(repo_id, &r);

    if (r.isValid()) {
        QString local_path = ::pathJoin(r.worktree, path_in_repo);

        FileDownloadHelper::openFile(local_path, false);
    } else {
        QString fixed_path = path_in_repo == "." ? "/" : path_in_repo;
        ServerRepo repo = getRepo(repo_id);
        if (!repo.isValid()) {
            QString msg = tr("Unable to open file \"%1\" from nonexistent library \"%2\"").arg(path_in_repo).arg(repo_id);
            seafApplet->warningBox(msg);
            return;
        }

        //TODO select the correct account
        const Account account = seafApplet->accountManager()->currentAccount();
        if (!account.isValid()) {
            qWarning("no valid account found");
            return;
        }
        FileBrowserManager::getInstance()->openOrActivateDialog(account, repo, fixed_path);
    }
}

void RepoService::onGetRequestSuccess(const ServerRepo& repo)
{
    GetRepoRequest* req = qobject_cast<GetRepoRequest*>(sender());
    if (!req)
        return;

    SyncedSubfolderhasRepoID subfolder_helper(repo.id);
    std::vector<SyncedSubfolder>::iterator pos = std::find_if(synced_subfolders_.begin(), synced_subfolders_.end(), subfolder_helper);

    if (pos != synced_subfolders_.end()) {
        ServerRepo fixed_repo = repo;
        fixed_repo.parent_path = pos->parentPath();
        fixed_repo.parent_repo_id = pos->parentRepoId();
        server_repos_.push_back(fixed_repo);
    }

    // delete current request
    req->deleteLater();
    get_repo_reqs_.pop_front();

    // start the next request or mark it as success
    if (!get_repo_reqs_.empty())
        get_repo_reqs_.front()->send();
    else
        emit refreshSuccess(server_repos_);
}

void RepoService::onGetRequestFailed(const ApiError& /*error*/)
{
    GetRepoRequest* req = qobject_cast<GetRepoRequest*>(sender());
    if (!req)
        return;

    // delete current request
    req->deleteLater();
    get_repo_reqs_.pop_front();

    // start the next request or mark it as success
    if (!get_repo_reqs_.empty())
        get_repo_reqs_.front()->send();
    else
        emit refreshSuccess(server_repos_);
}

void RepoService::startGetRequestFor(const QString &repo_id)
{
    bool was_empty = get_repo_reqs_.empty();
    GetRepoRequest *req = new GetRepoRequest(seafApplet->accountManager()->currentAccount(), repo_id);
    get_repo_reqs_.push_back(req);
    connect(req, SIGNAL(success(const ServerRepo&)), this, SLOT(onGetRequestSuccess(const ServerRepo&)));
    connect(req, SIGNAL(failed(const ApiError&)), this, SLOT(onGetRequestFailed(const ApiError&)));
    if (was_empty)
        req->send();
}

void RepoService::saveSyncedSubfolder(const ServerRepo& subfolder)
{
    const std::vector<Account>& accounts = seafApplet->accountManager()->accounts();
    if (accounts.empty() || !subfolder.isSubfolder())
        return;
    const Account *account = &accounts.front();

    // if we have it before
    RepohasRepoID<ServerRepo> repo_helper(subfolder.id);
    std::vector<ServerRepo>::iterator pos = std::find_if(server_repos_.begin(), server_repos_.end(), repo_helper);
    if (pos == server_repos_.end()) {
        server_repos_.push_back(subfolder);
    } else {
        pos->parent_path = subfolder.parent_path;
        pos->parent_repo_id = subfolder.parent_repo_id;
    }

    if (!synced_subfolder_db_) {
        qWarning("synced subfolder database is not available\n");
        return;
    }

    char *zql = sqlite3_mprintf(
        "REPLACE INTO SyncedSubfolder(repo_id, parent_repo_id, url, username, "
        "parent_path) VALUES (%Q, %Q, %Q, %Q, %Q)",
        subfolder.id.toUtf8().data(), subfolder.parent_repo_id.toUtf8().data(),
        account->serverUrl.toEncoded().data(),
        account->username.toUtf8().data(),
        subfolder.parent_path.toUtf8().data());
    sqlite_query_exec (synced_subfolder_db_, zql);
    sqlite3_free(zql);

    emit refreshSuccess(server_repos_);
}

void RepoService::removeSyncedSubfolder(const QString& repo_id)
{
    char *zql = sqlite3_mprintf("DELETE FROM SyncedSubfolder WHERE repo_id = %Q", repo_id.toUtf8().data());
    sqlite_query_exec(synced_subfolder_db_, zql);
    sqlite3_free(zql);

    SyncedSubfolderhasRepoID subfolder_helper(repo_id);
    synced_subfolders_.erase(std::remove_if(synced_subfolders_.begin(), synced_subfolders_.end(), subfolder_helper), synced_subfolders_.end());

    RepohasRepoID<ServerRepo> repo_helper(repo_id);
    std::vector<ServerRepo>::iterator pos = std::remove_if(server_repos_.begin(), server_repos_.end(), repo_helper);
    if (pos != server_repos_.end() && pos->isSubfolder()) {
        server_repos_.erase(pos, server_repos_.end());

        emit refreshSuccess(server_repos_);
    }
}

void RepoService::removeCloudFileBrowserCache()
{
    QList<FileCache::CacheEntry> all_files =
        FileCache::instance()->getAllCachedFiles();
    const Account account = seafApplet->accountManager()->currentAccount();
    foreach (const FileCache::CacheEntry& entry, all_files) {
        if (account.getSignature() == entry.account_sig) {
            QString fullpath = DataManager::getLocalCacheFilePath(entry.repo_id, entry.path);
            printf ("removing cached file %s\n", toCStr(fullpath));
            ::unlink((const char*)(toCStr(fullpath)));
        }
    }
}


void WipeFilesThread::run()
{
    for (size_t i = 0; i < local_repos_.size(); ++i) {
        const LocalRepo& repo = local_repos_[i];
        qWarning ("removing all files of repo %s: %s\n",
                  toCStr(repo.name),
                  toCStr(repo.worktree));
        if (repo.worktree.length() > 1) {
            delete_dir_recursively(repo.worktree);
        }
    }

    for (int i = 0; i < cached_files_.size(); i++) {
        ::unlink(toCStr(cached_files_[i]));
    }

    emit done();
}

void RepoService::wipeLocalFiles()
{
    const Account& account = seafApplet->accountManager()->currentAccount();

    // Collect repo worktrees
    std::vector<LocalRepo> repos_to_wipe;
    seafApplet->rpcClient()->listLocalRepos(&local_repos_);
    QStringList worktrees;
    for (size_t i = 0; i < local_repos_.size(); ++i) {
        const LocalRepo& repo = local_repos_[i];

        QString repo_server_url;
        if (seafApplet->rpcClient()->getRepoProperty(repo.id, "server-url", &repo_server_url) < 0) {
            continue;
        }
        if (QUrl(repo_server_url).host() != account.serverUrl.host()) {
            continue;
        }

        seafApplet->rpcClient()->unsync(repo.id);
        repos_to_wipe.push_back(repo);
    }

    // Collect files cached by cloud file browser
    QStringList cached_files;
    QList<FileCache::CacheEntry> all_files =
        FileCache::instance()->getAllCachedFiles();
    foreach (const FileCache::CacheEntry& entry, all_files) {
        if (account.getSignature() == entry.account_sig) {
            QString fullpath = DataManager::getLocalCacheFilePath(entry.repo_id, entry.path);
            cached_files << DataManager::getLocalCacheFilePath(entry.repo_id, entry.path);
        }
    }

    WipeFilesThread *wiper = new WipeFilesThread(repos_to_wipe, cached_files);
    connect(wiper, SIGNAL(done()), this, SLOT(onWiperDone()));

    QThreadPool::globalInstance()->start(wiper);
}

void RepoService::onWiperDone()
{
    RemoteWipeReportRequest *req = new RemoteWipeReportRequest(
        seafApplet->accountManager()->currentAccount());

    connect(req,
            SIGNAL(success()),
            this,
            SLOT(onRemoteWipeReportSuccess()));

    connect(req,
            SIGNAL(failed(const ApiError &)),
            this,
            SLOT(onRemoteWipeReportFailed(const ApiError &)));

    req->send();
}

void RepoService::onRemoteWipeReportSuccess()
{
    wipe_in_progress_ = false;
    RemoteWipeReportRequest* req = qobject_cast<RemoteWipeReportRequest*>(sender());
    req->deleteLater();
    seafApplet->accountManager()->invalidateCurrentLogin();
}

void RepoService::onRemoteWipeReportFailed(const ApiError& error)
{
    wipe_in_progress_ = false;
    RemoteWipeReportRequest* req = qobject_cast<RemoteWipeReportRequest*>(sender());
    req->deleteLater();
    seafApplet->accountManager()->invalidateCurrentLogin();
}
