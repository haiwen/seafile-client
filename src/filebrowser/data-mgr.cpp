#include <errno.h>
#include <cstdio>
#include <sqlite3.h>

#include <QDir>
#include <QDateTime>

#include "utils/file-utils.h"
#include "utils/utils.h"
#include "configurator.h"
#include "seafile-applet.h"
#include "auto-update-mgr.h"
#include "api/requests.h"
#include "repo-service.h"

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

DataManager::DataManager(const Account &account)
    : account_(account),
      filecache_db_(FileCacheDB::instance()),
      dirents_cache_(DirentsCache::instance())
{
}

DataManager::~DataManager()
{
    Q_FOREACH(SeafileApiRequest *req, reqs_)
    {
        delete req;
    }
}

bool DataManager::getDirents(const QString& repo_id,
                             const QString& path,
                             QList<SeafDirent> *dirents)
{
    QList<SeafDirent> *l = dirents_cache_->getCachedDirents(repo_id, path);
    if (l != NULL) {
        dirents->append(*l);
        return true;
    } else {
        return false;
    }
}

void DataManager::getDirentsFromServer(const QString& repo_id,
                                       const QString& path)
{
    get_dirents_req_.reset(new GetDirentsRequest(account_, repo_id, path));
    connect(get_dirents_req_.data(), SIGNAL(success(const QList<SeafDirent>&)),
            this, SLOT(onGetDirentsSuccess(const QList<SeafDirent>&)));
    connect(get_dirents_req_.data(), SIGNAL(failed(const ApiError&)),
            this, SIGNAL(getDirentsFailed(const ApiError&)));
    get_dirents_req_->send();
}

void DataManager::createDirectory(const QString &repo_id,
                                  const QString &path)
{
    CreateDirectoryRequest *req = new CreateDirectoryRequest(account_, repo_id, path);
    connect(req, SIGNAL(success()),
            SLOT(onCreateDirectorySuccess()));

    connect(req, SIGNAL(failed(const ApiError&)),
            SIGNAL(createDirectoryFailed(const ApiError&)));

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
    connect(req, SIGNAL(success()),
            SLOT(onRenameDirentSuccess()));

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
    connect(req, SIGNAL(success()),
            SLOT(onRemoveDirentSuccess()));

    connect(req, SIGNAL(failed(const ApiError&)),
            SIGNAL(removeDirentFailed(const ApiError&)));
    req->send();
    reqs_.push_back(req);
}

void DataManager::shareDirent(const QString &repo_id,
                              const QString &path,
                              bool is_file)
{
    GetSharedLinkRequest *req = new GetSharedLinkRequest(account_, repo_id,
                                                         path, is_file);
    connect(req, SIGNAL(success(const QString&)),
            SIGNAL(shareDirentSuccess(const QString&)));

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
    connect(req, SIGNAL(success()),
            SLOT(onCopyDirentsSuccess()));

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
    connect(req, SIGNAL(success()),
            SLOT(onMoveDirentsSuccess()));

    connect(req, SIGNAL(failed(const ApiError&)),
            SIGNAL(moveDirentsFailed(const ApiError&)));
    reqs_.push_back(req);
    req->send();
}

void DataManager::onGetDirentsSuccess(const QList<SeafDirent> &dirents)
{
    dirents_cache_->saveCachedDirents(get_dirents_req_->repoId(),
                                      get_dirents_req_->path(),
                                      dirents);

    emit getDirentsSuccess(dirents);
}

void DataManager::onCreateDirectorySuccess()
{
    CreateDirectoryRequest *req = qobject_cast<CreateDirectoryRequest*>(sender());

    if(req == NULL)
        return;

    removeDirentsCache(req->repoId(), req->path(), false);
    emit createDirectorySuccess(req->path());
}

void DataManager::onRenameDirentSuccess()
{
    RenameDirentRequest *req = qobject_cast<RenameDirentRequest*>(sender());

    if(req == NULL)
        return;

    removeDirentsCache(req->repoId(), req->path(), req->isFile());
    emit renameDirentSuccess(req->path(), req->newName());
}

void DataManager::onRemoveDirentSuccess()
{
    RemoveDirentRequest *req = qobject_cast<RemoveDirentRequest*>(sender());

    if(req == NULL)
        return;

    removeDirentsCache(req->repoId(), req->path(), req->isFile());
    emit removeDirentSuccess(req->path());
}

void DataManager::onCopyDirentsSuccess()
{
    emit copyDirentsSuccess();
}

void DataManager::onMoveDirentsSuccess()
{
    MoveMultipleFilesRequest *req = qobject_cast<MoveMultipleFilesRequest*>(sender());
    dirents_cache_->expireCachedDirents(req->srcRepoId(), req->srcPath());

    emit moveDirentsSuccess();
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
    if (!QFileInfo(local_file_path).exists()) {
        return "";
    }

    QString cached_file_id = filecache_db_->getCachedFileId(repo_id, fpath);
    return cached_file_id == file_id ? local_file_path : "";
}

FileDownloadTask* DataManager::createDownloadTask(const QString& repo_id,
                                                  const QString& path)
{
    QString local_path = getLocalCacheFilePath(repo_id, path);
    FileDownloadTask* task = TransferManager::instance()->addDownloadTask(
        account_, repo_id, path, local_path);
    connect(task, SIGNAL(finished(bool)),
            this, SLOT(onFileDownloadFinished(bool)), Qt::UniqueConnection);

    return task;
}

void DataManager::onFileDownloadFinished(bool success)
{
    FileDownloadTask *task = qobject_cast<FileDownloadTask *>(sender());
    if (task == NULL)
        return;
    if (success) {
        filecache_db_->saveCachedFileId(task->repoId(),
                                        task->path(),
                                        task->fileId(),
                                        account_.getSignature());
        AutoUpdateManager::instance()->watchCachedFile(
            account_, task->repoId(), task->path());
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

    return task;
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

QHash<QString, qint64> DataManager::passwords_cache_;

bool DataManager::isRepoPasswordSet(const QString& repo_id) const
{
    if (!passwords_cache_.contains(repo_id)) {
        return false;
    }
    qint64 expiration_time = passwords_cache_[repo_id];
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    return now < expiration_time;
}

void DataManager::setRepoPasswordSet(const QString& repo_id)
{
    passwords_cache_[repo_id] =
        QDateTime::currentMSecsSinceEpoch() + kPasswordCacheExpirationMSecs;
}

QString DataManager::getRepoCacheFolder(const QString& repo_id) const
{
    QString seafdir = seafApplet->configurator()->seafileDir();
    return ::pathJoin(seafdir, kFileCacheTopDirName, repo_id);
}

void DataManager::createSubrepo(const QString &name, const QString& repo_id, const QString &path, const QString &password)
{
    //TODO fix password?
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
        emit createSubrepoSuccess(fixed_repo);
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
        emit createSubrepoSuccess(fixed_repo);
        return;
    }

    // it is not expected
    emit createSubrepoFailed(ApiError::fromHttpError(500));
}
