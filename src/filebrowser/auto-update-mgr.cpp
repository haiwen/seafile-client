#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QDateTime>
#include <QStringList>
#include <QTimer>
#include <QThreadPool>

#include "api/requests.h"
#include "seafile-applet.h"
#include "ui/tray-icon.h"
#include "configurator.h"
#include "account-mgr.h"
#include "rpc/rpc-client.h"
#include "rpc/local-repo.h"
#include "utils/file-utils.h"
#include "utils/utils.h"
#include "utils/uninstall-helpers.h"
#include "data-mgr.h"
#include "tasks.h"
#include "transfer-mgr.h"

#include "auto-update-mgr.h"
#include "repo-service.h"

namespace {

const char *kFileCacheTopDirName = "file-cache";
const char *kFileCacheTempTopDirName = "file-cache-tmp";

inline bool addPath(QFileSystemWatcher *watcher, const QString &file) {
  if (watcher->files().contains(file))
      return true;
  bool ret = watcher->addPath(file);
  if (!ret) {
      qWarning("[AutoUpdateManager] failed to watch cache file %s", file.toUtf8().data());
  }
  return ret;
}

inline bool removePath(QFileSystemWatcher *watcher, const QString &file) {
  if (!watcher->files().contains(file))
      return true;
  bool ret = watcher->removePath(file);
  if (!ret) {
      qWarning("[AutoUpdateManager] failed to remove watch on cache file %s", file.toUtf8().data());
  }
  return ret;
}
} // anonymous namespace

SINGLETON_IMPL(AutoUpdateManager)


AutoUpdateManager::AutoUpdateManager()
{
    system_shut_down_ = false;
    connect(&watcher_, SIGNAL(fileChanged(const QString&)),
            this, SLOT(onFileChanged(const QString&)));
    connect(qApp, SIGNAL(aboutToQuit()),
            this, SLOT(systemShutDown()));
}

void AutoUpdateManager::systemShutDown()
{
    system_shut_down_ = true;
}

void AutoUpdateManager::start()
{
    cleanCachedFile();
}

void AutoUpdateManager::watchCachedFile(const Account& account,
                                        const QString& repo_id,
                                        const QString& path)
{
    QString local_path = DataManager::getLocalCacheFilePath(repo_id, path);
    if (!QFileInfo(local_path).exists()) {
        return;
    }

    // do we have it in deferred list ?
    // skip if yes
    Q_FOREACH(const WatchedFileInfo& info, deleted_files_infos_)
    {
        if (repo_id == info.repo_id && path == info.path_in_repo)
            return;
    }
    Q_FOREACH(const WatchedFileInfo& info, watch_infos_)
    {
        if (repo_id == info.repo_id && path == info.path_in_repo)
            return;
    }

    addPath(&watcher_, local_path);

    QFileInfo finfo(local_path);
    watch_infos_[local_path] =
        WatchedFileInfo(account,
                        repo_id,
                        path,
                        finfo.lastModified().toMSecsSinceEpoch(),
                        finfo.size());
}

void AutoUpdateManager::cleanCachedFile()
{
    qWarning("[AutoUpdateManager] cancel all download tasks");
    TransferManager::instance()->cancelAllDownloadTasks();

    const Account cur_account = seafApplet->accountManager()->currentAccount();
    foreach(const QString& key, watch_infos_.keys()) {
        if (watch_infos_[key].account == cur_account)
            watch_infos_.remove(key);
    }

    qWarning("[AutoUpdateManager] clean file caches db");
    RepoService::instance()->removeCloudFileBrowserCache();

    qWarning("[AutoUpdateManager] clean file caches");
    CachedFilesCleaner *cleaner = new CachedFilesCleaner();
    QThreadPool::globalInstance()->start(cleaner);
}

void AutoUpdateManager::uploadFile(const QString& local_path, bool replace_previous)
{
    WatchedFileInfo &info = watch_infos_[local_path];

    FileCache::CacheEntry entry;
    FileCache::instance()->getCacheEntry(info.repo_id, info.path_in_repo, &entry);

    // If the replace_previous is true, entry.commit_id will be ignored to
    // force replace existing files on server. Otherwise, a conflict file may
    // be generated.
    auto commit_id = replace_previous ? "" : entry.commit_id;

    FileNetworkTask *task = seafApplet->dataManager()->createUploadTask(
        info.repo_id, ::getParentPath(info.path_in_repo), local_path,
        commit_id, ::getBaseName(local_path), true);

    ((FileUploadTask *)task)->setAcceptUserConfirmation(false);

    connect(task, SIGNAL(finished(bool)),
            this, SLOT(onUpdateTaskFinished(bool)));

    info.uploading = true;

    task->start();
}

void AutoUpdateManager::onFileChanged(const QString& local_path)
{
    qInfo() << "[AutoUpdateManager] file changed" << local_path;

    if (!watch_infos_.contains(local_path)) {
        // filter unwanted events
        return;
    }

    if (remote_changed_files_.contains(local_path)) {
        qInfo() << "[AutoUpdateManager] remote changed file" << local_path
                << "is not synced yet, skip this event";
        return;
    }

    WatchedFileInfo &info = watch_infos_[local_path];
    QFileInfo finfo(local_path);

    // Download the doc file in the mac will automatically upload
    // If the timestamp has not changed, it will not be uploaded
    qint64 mtime = finfo.lastModified().toMSecsSinceEpoch();
    if (mtime == info.mtime) {
        qInfo() << "[AutoUpdateManager] the mtime of file" << local_path
                << "has not changed, skip this event";
        return;
    }

#ifdef Q_OS_MAC
    if (MacImageFilesWorkAround::instance()->isRecentOpenedImage(local_path)) {
        qInfo("[AutoUpdateManager] skip the image file updates on mac for %s", toCStr(local_path));
        return;
    }
#endif
    removePath(&watcher_, local_path);
    QString repo_id, path_in_repo;

    if (!finfo.exists()) {
        qInfo() << "[AutoUpdateManager] file renamed or removed" << local_path;

        WatchedFileInfo deferred_info = info;
        removeWatch(local_path);
        // Some application would deleted and recreate the file when saving.
        // We work around that by double checking whether the file gets
        // recreated after a short period
        QTimer::singleShot(5000, this, SLOT(checkFileRecreated()));
        deleted_files_infos_.enqueue(deferred_info);
        return;
    }

    qInfo() << "[AutoUpdateManager] uploading file" << local_path
            << ", size =" << finfo.size() << ", mtime =" << finfo.lastModified();

    uploadFile(local_path, false);
}

void AutoUpdateManager::onUpdateTaskFinished(bool success)
{
    qDebug("on update task finished: %s", success ? "true" : "false");
    if (system_shut_down_) {
        return;
    }

    FileUploadTask *task = qobject_cast<FileUploadTask *>(sender());
    if (task == NULL)
        return;
    const QString local_path = task->localFilePath();
    const QFileInfo finfo = QFileInfo(local_path);
    if (!finfo.exists()) {
        //TODO: What if the delete&recreate happens just before this function is called?
        qWarning("[AutoUpdateManager] file %s not exists anymore", toCStr(local_path));
        return;
    }

    if (!watch_infos_.contains(local_path)) {
        qWarning("[AutoUpdateManager] no watch info for file %s", toCStr(local_path));
        return;
    }
    WatchedFileInfo& info = watch_infos_[local_path];
    info.uploading = false;

    if (!success) {
        qWarning("[AutoUpdateManager] failed to upload new version of file %s: %s",
                 toCStr(local_path),
                 toCStr(task->errorString()));
        QString error_msg;
        if (task->httpErrorCode() == 403) {
            error_msg = tr("Permission Error!");
        } else if (task->httpErrorCode() == 401) {
            error_msg = tr("Authorization expired");
        } else if (task->httpErrorCode() == 441) {
            error_msg = tr("File does not exist");
        } else if (task->httpErrorCode() == 442) {
            error_msg = tr("File size exceeds limit");
        } else if (task->httpErrorCode() == 447) {
            error_msg = tr("Number of file exceeds limit");
        } else {
            error_msg = task->errorString();
        }

        QString name = ::getBaseName(local_path);
        DirentsCache::ReturnEntry retval = DirentsCache::instance()->getCachedDirents(info.repo_id, task->path());
        QList<SeafDirent> *l = retval.second;
        QString msg = tr("File \"%1\"\nfailed to upload.").arg(QFileInfo(local_path).fileName());
        if (l != NULL) {
            foreach (const SeafDirent dirent, *l) {
                if (dirent.name == name) {
                    if (dirent.is_locked) {
                        msg = tr("The file is locked by %1, "
                                 "please try again later").arg(dirent.getLockOwnerDisplayString());
                    }
                }
            }
        }
        seafApplet->trayIcon()->showMessage(tr("Upload Failure: %1").arg(error_msg),
                                            msg,
                                            task->repoId());

        addPath(&watcher_, local_path);
        return;
    }

    QString repo_id    = task->repoId(),
            path       = info.path_in_repo,
            sig        = task->account().getSignature(),
            file_id    = task->oid();
    auto req = new GetRepoRequest(task->account(), repo_id);
    connect(req, SIGNAL(failed(const ApiError&)),
            this, SLOT(onGetRepoFailed(const ApiError&)));
    connect(req, &GetRepoRequest::success,
            this, [=](const ServerRepo& repo) {
                this->onGetRepoSuccess(repo, repo_id, path, sig, file_id, local_path);
            });
    req->send();
}

void AutoUpdateManager::onGetRepoFailed(const ApiError& error)
{
    qWarning() << "[AutoUpdateManager] failed to list repos:" << error.toString();
}

void AutoUpdateManager::onGetRepoSuccess(const ServerRepo& repo, QString repo_id, QString path, QString sig, QString file_id, QString local_path)
{
    // The commit_id should not be a null string.
    QString commit_id = repo.head_commit_id.isEmpty() ? "" : repo.head_commit_id;
    const QFileInfo finfo = QFileInfo(local_path);
    WatchedFileInfo& info = watch_infos_[local_path];

    qDebug("[AutoUpdateManager] uploaded new version of file %s", local_path.toUtf8().data());
    info.mtime = finfo.lastModified().toMSecsSinceEpoch();
    info.fsize = finfo.size();
    seafApplet->trayIcon()->showMessage(tr("Upload Success"),
                                        tr("File \"%1\"\nuploaded successfully.").arg(finfo.fileName()),
                                        repo_id);

    // This would also set the "uploading" and "num_upload_errors" column to 0.
    FileCache::instance()->saveCachedFileId(repo_id, path, sig, file_id, commit_id, local_path);

    emit fileUpdated(repo_id, path);

    addPath(&watcher_, local_path);
}

void AutoUpdateManager::removeWatch(const QString& local_path)
{
    watch_infos_.remove(local_path);
    removePath(&watcher_, local_path);
}

void AutoUpdateManager::checkFileRecreated()
{
    if (deleted_files_infos_.isEmpty()) {
        // impossible
        return;
    }

    const WatchedFileInfo info = deleted_files_infos_.dequeue();
    const QString path = DataManager::getLocalCacheFilePath(info.repo_id, info.path_in_repo);
    if (QFileInfo(path).exists()) {
        qInfo() << "[AutoUpdateManager] file recreated" << path;
        addPath(&watcher_, path);
        watch_infos_[path] = info;
        // Some applications like MSOffice would remove the original file and
        // recreate it when the user modifies the file.
        onFileChanged(path);
    }
}

QHash<QString, AutoUpdateManager::FileStatus>
AutoUpdateManager::getFileStatusForDirectory(const QString &account_sig,
                                             const QString &repo_id,
                                             const QString &parent_dir,
                                             const QList<SeafDirent>& dirents)
{
    QHash<QString, SeafDirent> dirents_map;
    foreach(const SeafDirent& d, dirents) {
        if (d.isFile()) {
            dirents_map[d.name] = d;
        }
    }

    QHash<QString, FileStatus> ret;
    QList<FileCache::CacheEntry> caches =
        FileCache::instance()->getCachedFilesForDirectory(account_sig, repo_id, parent_dir);

    foreach(const FileCache::CacheEntry& entry, caches) {
        QString local_file_path = DataManager::getLocalCacheFilePath(entry.repo_id, entry.path);
        const QString& file = ::getBaseName(entry.path);

        if (!dirents_map.contains(file)) {
            remote_changed_files_.remove(local_file_path);
            continue;
        }

        bool is_uploading = watch_infos_.contains(local_file_path) && watch_infos_[local_file_path].uploading;
        if (is_uploading) {
            remote_changed_files_.remove(local_file_path);
            ret[file] = UPLOADING;
            continue;
        }

        QFileInfo info(local_file_path);
        auto size = info.size();
        auto mtime = info.lastModified().toMSecsSinceEpoch();
        if (dirents_map[file].id != entry.file_id) {
            // If the remote file id is different from the cached one (which means the remote file is changed), we record it in the remote_changed_files_ and prevent auto uploading of this file.
            remote_changed_files_.insert(local_file_path);
            ret[file] = NOT_SYNCED;
        } else if (mtime == entry.seafile_mtime && size == entry.seafile_size) {
            remote_changed_files_.remove(local_file_path);
            ret[file] = SYNCED;
        } else {
            remote_changed_files_.remove(local_file_path);
            ret[file] = NOT_SYNCED;
        }
    }

    return ret;
}

#ifdef Q_OS_MAC
SINGLETON_IMPL(MacImageFilesWorkAround)

MacImageFilesWorkAround::MacImageFilesWorkAround()
{
}

void MacImageFilesWorkAround::fileOpened(const QString& path)
{
    QString mimetype = ::mimeTypeFromFileName(path);
    if (mimetype.startsWith("image") || mimetype == "application/pdf") {
        images_[path] = QDateTime::currentMSecsSinceEpoch();
    }
}

bool MacImageFilesWorkAround::isRecentOpenedImage(const QString& path)
{
    qint64 ts = images_.value(path, 0);
    if (QDateTime::currentMSecsSinceEpoch() < ts + 1000 * 10) {
        return true;
    } else {
        return false;
    }
}
#endif

CachedFilesCleaner::CachedFilesCleaner()
{
}

void CachedFilesCleaner::run()
{
    QString file_cache_dir = pathJoin(seafApplet->configurator()->seafileDir(),
                               kFileCacheTopDirName);
    QString file_cache_tmp_dir = pathJoin(seafApplet->configurator()->seafileDir(),
                                   kFileCacheTempTopDirName);

    qDebug("[AutoUpdateManager] removing cached files");
    if (QDir(file_cache_tmp_dir).exists()) {
        delete_dir_recursively(file_cache_tmp_dir);
    }
    if (QDir(file_cache_dir).exists()) {
        // Delete the temporary directory in the old client.
        if (QDir(file_cache_tmp_dir).exists()) {
            delete_dir_recursively(file_cache_tmp_dir);
        }
    }
}

void AutoUpdateManager::dumpCacheStatus()
{
    printf ("---------------BEGIN CACHE INFO -------------\n");
    foreach(const QString& key, watch_infos_.keys()) {
        WatchedFileInfo& info = watch_infos_[key];
        printf ("%s mtime = %lld, fsize = %lld, uploading = %s\n", toCStr(info.path_in_repo), info.mtime, info.fsize, info.uploading ? "true" : "false");
    }
    printf ("---------------END CACHE INFO -------------\n");
}
