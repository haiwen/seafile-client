#include <errno.h>
#include <cstdio>
#include <sqlite3.h>

#include <QDir>
#include <QDateTime>

#include "utils/file-utils.h"
#include "utils/utils.h"
#include "configurator.h"
#include "seafile-applet.h"
#include "rpc/rpc-client.h"
#include "auto-update-mgr.h"
#include "api/requests.h"
#include "repo-service.h"
#include "account-mgr.h"

#include "filebrowser/file-browser-requests.h"
#include "filebrowser/tasks.h"
#include "filebrowser/transfer-mgr.h"
#include "filebrowser/data-cache.h"
#include "filebrowser/data-mgr.h"

namespace {

const char *kFileCacheTopDirName = "file-cache";
const int kPasswordCacheExpirationMSecs = 30 * 60 * 1000;

} // namespace

/**
 * Cache loaded dirents. But default cache expires after 1 minute.
 */

SINGLETON_IMPL(DataManager)

DataManager::DataManager()
    : filecache_(FileCache::instance()),
      dirents_cache_(DirentsCache::instance())
{
}

DataManager::~DataManager()
{
    emit aboutToDestroy();
    Q_FOREACH(SeafileApiRequest *req, reqs_)
    {
        req->deleteLater();
    }
}

void DataManager::start()
{
    account_ = seafApplet->accountManager()->currentAccount();

    connect(seafApplet->accountManager(), SIGNAL(accountsChanged()),
            this, SLOT(onAccountChanged()));
}

bool DataManager::getDirents(const QString& repo_id,
                             const QString& path,
                             QList<SeafDirent> *dirents,
                             bool *current_readonly)
{
    DirentsCache::ReturnEntry retval = dirents_cache_->getCachedDirents(repo_id, path);
    QList<SeafDirent> *l = retval.second;
    if (l != NULL) {
        dirents->append(*l);
        *current_readonly = retval.first;
        return true;
    } else {
        return false;
    }
}

void DataManager::getDirentsFromServer(const QString& repo_id,
                                       const QString& path)
{
    GetDirentsRequest *get_dirents_req = new GetDirentsRequest(account_, repo_id, path);
    connect(get_dirents_req, SIGNAL(success(bool, const QList<SeafDirent>&, const QString&)),
            this, SLOT(onGetDirentsSuccess(bool, const QList<SeafDirent>&, const QString&)));
    connect(get_dirents_req, SIGNAL(failed(const ApiError&, const QString&)),
            this, SIGNAL(getDirentsFailed(const ApiError&, const QString&)));
    get_dirents_req->send();
    reqs_.push_back(get_dirents_req);
}

void DataManager::createDirectory(const QString &repo_id,
                                  const QString &path)
{
    CreateDirectoryRequest *req = new CreateDirectoryRequest(account_, repo_id, path);
    connect(req, SIGNAL(success(const QString&)),
            SLOT(onCreateDirectorySuccess(const QString&)));

    connect(req, SIGNAL(failed(const ApiError&)),
            SIGNAL(createDirectoryFailed(const ApiError&)));

    req->send();
    reqs_.push_back(req);
}

void DataManager::lockFile(const QString &repo_id,
                           const QString &path,
                           bool lock)
{
    LockFileRequest *req = new LockFileRequest(account_, repo_id, path, lock);
    connect(req, SIGNAL(success(const QString&)),
            SLOT(onLockFileSuccess(const QString&)));

    connect(req, SIGNAL(failed(const ApiError&)),
            SIGNAL(lockFileFailed(const ApiError&)));

    req->send();
    reqs_.push_back(req);
}

void DataManager::renameDirent(const QString &repo_id,
                               const QString &path,
                               const QString &new_path,
                               bool is_file)
{
    RenameDirentRequest *req = new RenameDirentRequest(account_, repo_id, path,
                                                       new_path, is_file);
    connect(req, SIGNAL(success(const QString&)),
            SLOT(onRenameDirentSuccess(const QString&)));

    connect(req, SIGNAL(failed(const ApiError&)),
            SIGNAL(renameDirentFailed(const ApiError&)));
    req->send();
    reqs_.push_back(req);
}


void DataManager::removeDirent(const QString &repo_id,
                               const QString &path,
                               bool is_file)
{
    RemoveDirentRequest *req = new RemoveDirentRequest(account_, repo_id, path,
                                                       is_file);
    connect(req, SIGNAL(success(const QString&)),
            SLOT(onRemoveDirentSuccess(const QString&)));

    connect(req, SIGNAL(failed(const ApiError&)),
            SIGNAL(removeDirentFailed(const ApiError&)));
    req->send();
    reqs_.push_back(req);
}

void DataManager::removeDirents(const QString &repo_id,
                                const QString &parent_path,
                                const QStringList &filenames)
{
    RemoveDirentsRequest *req = new RemoveDirentsRequest(account_, repo_id, parent_path,
                                                         filenames);
    connect(req, SIGNAL(success(const QString&)),
            SLOT(onRemoveDirentsSuccess(const QString&)));

    connect(req, SIGNAL(failed(const ApiError&)),
            SIGNAL(removeDirentsFailed(const ApiError&)));
    req->send();
    reqs_.push_back(req);
}

void DataManager::shareDirent(const QString &repo_id,
                              const QString &path,
                              bool is_file)
{
    GetSharedLinkRequest *req = new GetSharedLinkRequest(account_, repo_id,
                                                         path, is_file);
    connect(req, SIGNAL(success(const QString&, const QString&)),
            SIGNAL(shareDirentSuccess(const QString&, const QString&)));

    connect(req, SIGNAL(failed(const ApiError&)),
            SIGNAL(shareDirentFailed(const ApiError&)));
    reqs_.push_back(req);
    req->send();
}

void DataManager::copyDirents(const QString &repo_id,
                              const QString &dir_path,
                              const QStringList &file_names,
                              const QString &dst_repo_id,
                              const QString &dst_dir_path)
{
    CopyMultipleFilesRequest *req =
      new CopyMultipleFilesRequest(account_, repo_id, dir_path, file_names,
                                   dst_repo_id,
                                   dst_dir_path);
    connect(req, SIGNAL(success(const QString&)),
            SLOT(onCopyDirentsSuccess(const QString&)));

    connect(req, SIGNAL(failed(const ApiError&)),
            SIGNAL(copyDirentsFailed(const ApiError&)));
    reqs_.push_back(req);
    req->send();
}

void DataManager::moveDirents(const QString &repo_id,
                              const QString &dir_path,
                              const QStringList &file_names,
                              const QString &dst_repo_id,
                              const QString &dst_dir_path)
{
    MoveMultipleFilesRequest *req =
      new MoveMultipleFilesRequest(account_, repo_id, dir_path, file_names,
                                   dst_repo_id,
                                   dst_dir_path);
    connect(req, SIGNAL(success(const QString&)),
            SLOT(onMoveDirentsSuccess(const QString&)));

    connect(req, SIGNAL(failed(const ApiError&)),
            SIGNAL(moveDirentsFailed(const ApiError&)));
    reqs_.push_back(req);
    req->send();
}

void DataManager::onGetDirentsSuccess(bool current_readonly, const QList<SeafDirent> &dirents, const QString& repo_id)
{
    GetDirentsRequest *get_dirents_req =  qobject_cast<GetDirentsRequest *>(sender());
    dirents_cache_->saveCachedDirents(get_dirents_req->repoId(),
                                      get_dirents_req->path(),
                                      current_readonly,
                                      dirents);

    emit getDirentsSuccess(current_readonly, dirents, repo_id);
}

void DataManager::onCreateDirectorySuccess(const QString& repo_id)
{
    CreateDirectoryRequest *req = qobject_cast<CreateDirectoryRequest*>(sender());

    if(req == NULL)
        return;

    removeDirentsCache(req->repoId(), req->path(), false);
    emit createDirectorySuccess(req->path(), repo_id);
}

void DataManager::onLockFileSuccess(const QString& repo_id)
{
    LockFileRequest *req = qobject_cast<LockFileRequest *>(sender());
    if (!req)
        return;

    removeDirentsCache(req->repoId(), req->path(), false);
    seafApplet->rpcClient()->markFileLockState(req->repoId(), req->path(), req->lock());
    emit lockFileSuccess(req->path(), req->lock(), repo_id);
}

void DataManager::onRenameDirentSuccess(const QString& repo_id)
{
    RenameDirentRequest *req = qobject_cast<RenameDirentRequest*>(sender());

    if(req == NULL)
        return;

    removeDirentsCache(req->repoId(), req->path(), req->isFile());
    emit renameDirentSuccess(req->path(), req->newName(), repo_id);
}

void DataManager::onRemoveDirentSuccess(const QString& repo_id)
{
    RemoveDirentRequest *req = qobject_cast<RemoveDirentRequest*>(sender());

    if(req == NULL)
        return;

    removeDirentsCache(req->repoId(), req->path(), req->isFile());
    emit removeDirentSuccess(req->path(), repo_id);
}

void DataManager::onRemoveDirentsSuccess(const QString& repo_id)
{
    RemoveDirentsRequest *req = qobject_cast<RemoveDirentsRequest*>(sender());

    if(req == NULL)
        return;

    removeDirentsCache(req->repoId(), req->parentPath(), false);
    emit removeDirentsSuccess(req->parentPath(), req->filenames(), repo_id);
}

void DataManager::onCopyDirentsSuccess(const QString& dst_repo_id)
{
    emit copyDirentsSuccess(dst_repo_id);
}

void DataManager::onMoveDirentsSuccess(const QString& dst_repo_id)
{
    MoveMultipleFilesRequest *req = qobject_cast<MoveMultipleFilesRequest*>(sender());
    dirents_cache_->expireCachedDirents(req->srcRepoId(), req->srcPath());

    emit moveDirentsSuccess(dst_repo_id);
}

void DataManager::removeDirentsCache(const QString& repo_id,
                                     const QString& path,
                                     bool is_file)
{
    // expire its parent's cache
    dirents_cache_->expireCachedDirents(repo_id, ::getParentPath(path));
    // if the object is a folder, then expire its self cache
    if (!is_file)
        dirents_cache_->expireCachedDirents(repo_id, path);
}


QString DataManager::getLocalCachedFile(const QString& repo_id,
                                        const QString& fpath,
                                        const QString& file_id)
{
    QString local_file_path = getLocalCacheFilePath(repo_id, fpath);
    QFileInfo finfo(local_file_path);
    if (!finfo.exists()) {
        qWarning ("No cache file for %s\n", toCStr(fpath));
        return "";
    }

    FileCache::CacheEntry entry;
    if (!filecache_->getCacheEntry(repo_id, fpath, &entry)) {
        qWarning ("No cache db entry for %s\n", toCStr(fpath));
        return "";
    }

    if (entry.file_id == file_id) {
        qWarning ("cache file id matched for %s: %s\n", toCStr(fpath), toCStr(file_id));
        return local_file_path;
    } else {
        // The file is updated on server
        qint64 mtime = finfo.lastModified().toMSecsSinceEpoch();
        bool use_cached = false;
        if (mtime != entry.seafile_mtime) {
            qWarning(
                "cache file is updated locally (mtime changed from %lld to "
                "%lld), use it: %s\n",
                entry.seafile_mtime,
                mtime,
                toCStr(fpath));
            use_cached = true;
        } else if (finfo.size() != entry.seafile_size) {
            qWarning(
                "cache file is updated locally (size changed from %lld to "
                "%lld), use it: %s\n",
                entry.seafile_size,
                finfo.size(),
                toCStr(fpath));
            use_cached = true;
        }

        if (use_cached) {
            // If the file is also updated locally, open it directly
            return local_file_path;
        } else {
            qWarning ("cache file is outdated, download newer version: %s\n", toCStr(fpath));
            // Otherwise the newer version would be downloaded
            return "";
        }
    }
}

FileDownloadTask* DataManager::createDownloadTask(const QString& repo_id,
                                                  const QString& path)
{
    QString local_path = getLocalCacheFilePath(repo_id, path);
    FileDownloadTask* task = TransferManager::instance()->addDownloadTask(
        account_, repo_id, path, local_path);
    connect(task, SIGNAL(finished(bool)),
            this, SLOT(onFileDownloadFinished(bool)), Qt::UniqueConnection);
    setupTaskCleanup(task);

    return task;
}

FileDownloadTask* DataManager::createSaveAsTask(const QString& repo_id,
                                                const QString& path,
                                                const QString& local_path)
{
    FileDownloadTask* task = TransferManager::instance()->addDownloadTask(
        account_, repo_id, path, local_path, true);
    setupTaskCleanup(task);

    return task;
}

void DataManager::onFileDownloadFinished(bool success)
{
    FileDownloadTask *task = qobject_cast<FileDownloadTask *>(sender());
    if (task == NULL)
        return;
    if (success) {
        filecache_->saveCachedFileId(task->repoId(),
                                     task->path(),
                                     account_.getSignature(),
                                     task->fileId(),
                                     task->localFilePath());
        // TODO we don't want to watch readonly files
        AutoUpdateManager::instance()->watchCachedFile(account_, task->repoId(), task->path());
    }
}

FileUploadTask* DataManager::createUploadTask(const QString& repo_id,
                                              const QString& parent_dir,
                                              const QString& local_path,
                                              const QString& name,
                                              const bool overwrite)
{
    FileUploadTask *task;
    if (QFileInfo(local_path).isFile())
        task = new FileUploadTask(account_, repo_id, parent_dir,
                                  local_path, name, !overwrite);
    else
        task = new FileUploadDirectoryTask(account_, repo_id, parent_dir,
                                           local_path, name);
    connect(task, SIGNAL(finished(bool)),
            this, SLOT(onFileUploadFinished(bool)));
    setupTaskCleanup(task);

    return task;
}

void DataManager::setupTaskCleanup(FileNetworkTask *task)
{
    connect(this, SIGNAL(aboutToDestroy()),
            task, SLOT(cancel()));
}

FileUploadTask* DataManager::createUploadMultipleTask(const QString& repo_id,
                                                      const QString& parent_dir,
                                                      const QString& local_path,
                                                      const QStringList& names,
                                                      const bool overwrite)
{
    FileUploadTask *task = new FileUploadMultipleTask(account_, repo_id, parent_dir,
                                                      local_path, names, !overwrite);

    connect(task, SIGNAL(finished(bool)),
            this, SLOT(onFileUploadFinished(bool)));
    setupTaskCleanup(task);

    return task;
}

void DataManager::onFileUploadFinished(bool success)
{
    FileUploadTask *task = qobject_cast<FileUploadTask *>(sender());
    if (task == NULL)
        return;
    if (success) {
        //expire the parent path
        dirents_cache_->expireCachedDirents(task->repoId(),
                                            task->path());
    }
}

QString DataManager::getLocalCacheFilePath(const QString& repo_id,
                                           const QString& path)
{
    QString seafdir = seafApplet->configurator()->seafileDir();
    return ::pathJoin(seafdir, kFileCacheTopDirName, repo_id, path);
}

QHash<QString, std::pair<qint64, QString> > DataManager::passwords_cache_;

bool DataManager::isRepoPasswordSet(const QString& repo_id) const
{
    if (!passwords_cache_.contains(repo_id)) {
        return false;
    }
    qint64 expiration_time = passwords_cache_[repo_id].first;
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    return now < expiration_time;
}

void DataManager::setRepoPasswordSet(const QString& repo_id, const QString& password)
{
    passwords_cache_[repo_id] =
        std::pair<qint64, QString>(QDateTime::currentMSecsSinceEpoch() + kPasswordCacheExpirationMSecs, password);
}

QString DataManager::getRepoCacheFolder(const QString& repo_id) const
{
    QString seafdir = seafApplet->configurator()->seafileDir();
    return ::pathJoin(seafdir, kFileCacheTopDirName, repo_id);
}

void DataManager::createSubrepo(const QString &name, const QString& repo_id, const QString &path)
{
    old_repo_id_ = repo_id;
    const QString password = repoPassword(repo_id);
    const QString fixed_path = path.left(path.endsWith('/') && path.size() != 1 ? path.size() -1 : path.size());
    create_subrepo_req_.reset(new CreateSubrepoRequest(account_, name, repo_id, fixed_path, password));
    // we might have cleaned this value when do a new request while old request is still there
    get_repo_req_.reset(NULL);
    create_subrepo_parent_repo_id_ = repo_id;
    create_subrepo_parent_path_ = fixed_path;

    connect(create_subrepo_req_.data(), SIGNAL(success(const QString&)),
            this, SLOT(onCreateSubrepoSuccess(const QString&)));
    connect(create_subrepo_req_.data(), SIGNAL(failed(const ApiError&)),
            this, SIGNAL(createSubrepoFailed(const ApiError&)));

    create_subrepo_req_->send();
}

void DataManager::onCreateSubrepoSuccess(const QString& new_repoid)
{
    // if we have it, we are lucky
    ServerRepo repo = RepoService::instance()->getRepo(new_repoid);
    if (repo.isValid()) {
        ServerRepo fixed_repo = repo;
        fixed_repo.parent_path = create_subrepo_parent_path_;
        fixed_repo.parent_repo_id = create_subrepo_parent_repo_id_;
        emit createSubrepoSuccess(fixed_repo, old_repo_id_);
        return;
    }

    // if not found, we need call get repo (list repo is not reliable here)
    get_repo_req_.reset(new GetRepoRequest(account_, new_repoid));

    // connect
    connect(get_repo_req_.data(), SIGNAL(success(const ServerRepo&)),
            this, SLOT(onCreateSubrepoRefreshSuccess(const ServerRepo&)));
    connect(get_repo_req_.data(), SIGNAL(failed(const ApiError&)),
            this, SIGNAL(createSubrepoFailed(const ApiError&)));

    get_repo_req_->send();
}

void DataManager::onCreateSubrepoRefreshSuccess(const ServerRepo& repo)
{
    // okay, all green
    if (repo.isValid()) {
        ServerRepo fixed_repo = repo;
        fixed_repo.parent_path = create_subrepo_parent_path_;
        fixed_repo.parent_repo_id = create_subrepo_parent_repo_id_;
        emit createSubrepoSuccess(fixed_repo, old_repo_id_);
        return;
    }

    // it is not expected
    emit createSubrepoFailed(ApiError::fromHttpError(500));
}

void DataManager::onAccountChanged()
{
    account_ = seafApplet->accountManager()->currentAccount();
}
