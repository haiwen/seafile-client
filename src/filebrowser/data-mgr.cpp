#include <QDir>
#include <sqlite3.h>
#include <errno.h>
#include <stdio.h>
#include <QDateTime>

#include "utils/file-utils.h"
#include "utils/utils.h"
#include "configurator.h"
#include "seafile-applet.h"
#include "file-browser-requests.h"
#include "tasks.h"

#include "data-cache.h"
#include "data-mgr.h"

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

void DataManager::onGetDirentsSuccess(const QList<SeafDirent> &dirents)
{
    dirents_cache_->saveCachedDirents(get_dirents_req_->repoId(),
                                      get_dirents_req_->path(),
                                      dirents);

    emit getDirentsSuccess(dirents);
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

FileDownloadTask* DataManager::createDownloadTask(const ServerRepo& repo,
                                                  const QString& path)
{
    QString local_path = getLocalCacheFilePath(repo.id, path);
    FileDownloadTask *task = new FileDownloadTask(account_, repo, path, local_path);
    connect(task, SIGNAL(finished(bool)),
            this, SLOT(onFileDownloadFinished(bool)));

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
                                        task->fileId());
    }
}

FileUploadTask* DataManager::createUploadTask(const ServerRepo& repo,
                                              const QString& parent_dir,
                                              const QString& local_path,
                                              const QString& name,
                                              const bool overwrite)
{
    FileUploadTask *task = new FileUploadTask(account_, repo, parent_dir,
                                              local_path, name, !overwrite);
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
