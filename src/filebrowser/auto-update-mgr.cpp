#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QDateTime>
#include <QTimer>
#include <QThreadPool>

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
    qDebug("[AutoUpdateManager] watch cache file %s", local_path.toUtf8().data());
    if (!QFileInfo(local_path).exists()) {
        qWarning("[AutoUpdateManager] unable to watch non-existent cache file %s", local_path.toUtf8().data());
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
    FileCache::instance()->cleanCurrentAccountCache();

    qWarning("[AutoUpdateManager] clean file caches");
    CachedFilesCleaner *cleaner = new CachedFilesCleaner();
    QThreadPool::globalInstance()->start(cleaner);
}

void AutoUpdateManager::uploadFile(const QString& local_path)
{
    WatchedFileInfo &info = watch_infos_[local_path];

    FileNetworkTask *task = seafApplet->dataManager()->createUploadTask(
        info.repo_id, ::getParentPath(info.path_in_repo),
        local_path, ::getBaseName(local_path), true);

    ((FileUploadTask *)task)->setAcceptUserConfirmation(false);

    connect(task, SIGNAL(finished(bool)),
            this, SLOT(onUpdateTaskFinished(bool)));

    qDebug("[AutoUpdateManager] start uploading new version of file %s", local_path.toUtf8().data());

    info.uploading = true;

    task->start();
}

void AutoUpdateManager::onFileChanged(const QString& local_path)
{
    qDebug("[AutoUpdateManager] detected cache file %s changed", local_path.toUtf8().data());
    if (!watch_infos_.contains(local_path)) {
        // filter unwanted events
        return;
    }

    WatchedFileInfo &info = watch_infos_[local_path];
    QFileInfo finfo(local_path);

    // Download the doc file in the mac will automatically upload
    // If the timestamp has not changed, it will not be uploaded
    qint64 mtime = finfo.lastModified().toMSecsSinceEpoch();
    if (mtime == info.mtime) {
        qDebug("[AutoUpdateManager] Received a file %s upload notification, but the timestamp has not changed, "
               "it will not upload", local_path.toUtf8().data());
        return;
    }

#ifdef Q_OS_MAC
    if (MacImageFilesWorkAround::instance()->isRecentOpenedImage(local_path)) {
        qDebug("[AutoUpdateManager] skip the image file updates on mac for %s", toCStr(local_path));
        return;
    }
#endif
    removePath(&watcher_, local_path);
    QString repo_id, path_in_repo;

    if (!finfo.exists()) {
        qDebug("[AutoUpdateManager] detected cache file %s renamed or removed", local_path.toUtf8().data());
        WatchedFileInfo deferred_info = info;
        removeWatch(local_path);
        // Some application would deleted and recreate the file when saving.
        // We work around that by double checking whether the file gets
        // recreated after a short period
        QTimer::singleShot(5000, this, SLOT(checkFileRecreated()));
        deleted_files_infos_.enqueue(deferred_info);
        return;
    }

    uploadFile(local_path);
}

void AutoUpdateManager::onUpdateTaskFinished(bool success)
{
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

    if (success) {
        qDebug("[AutoUpdateManager] uploaded new version of file %s", local_path.toUtf8().data());
        info.mtime = finfo.lastModified().toMSecsSinceEpoch();
        info.fsize = finfo.size();
        seafApplet->trayIcon()->showMessage(tr("Upload Success"),
                                            tr("File \"%1\"\nuploaded successfully.").arg(finfo.fileName()),
                                            task->repoId());

        // This would also set the "uploading" and "num_upload_errors" column to 0.
        FileCache::instance()->saveCachedFileId(task->repoId(),
                                                info.path_in_repo,
                                                task->account().getSignature(),
                                                task->oid(),
                                                task->localFilePath());
        emit fileUpdated(task->repoId(), task->path());
    } else {
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
    }

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
        qDebug("[AutoUpdateManager] detected recreated file %s", path.toUtf8().data());
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
    if (caches.empty()) {
        // qDebug("no cached files for dir %s\n", toCStr(parent_dir));
    }
    foreach(const FileCache::CacheEntry& entry, caches) {
        // qDebug("found cache entry: %s\n", entry.path.toUtf8().data());

        QString local_file_path = DataManager::getLocalCacheFilePath(entry.repo_id, entry.path);
        const QString& file = ::getBaseName(entry.path);
        bool is_uploading = watch_infos_.contains(local_file_path) && watch_infos_[local_file_path].uploading;

        if (!dirents_map.contains(file)) {
            // qDebug("cached files no longer exists: %s\n", entry.path.toUtf8().data());
            continue;
        }

        const SeafDirent& d = dirents_map[file];
        if (d.id != entry.file_id) {
            // qDebug("cached file is a stale version: %s\n", entry.path.toUtf8().data());
            ret[file] = is_uploading ? UPLOADING : NOT_SYNCED;
            continue;
        }

        QFileInfo finfo(local_file_path);

        qint64 mtime = finfo.lastModified().toMSecsSinceEpoch();
        bool consistent = mtime == entry.seafile_mtime && finfo.size() == entry.seafile_size;
        if (consistent) {
            ret[file] = is_uploading ? UPLOADING: SYNCED;
        } else {
            ret[file] = is_uploading ? UPLOADING : NOT_SYNCED;
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
        QDir().rename(file_cache_dir, file_cache_tmp_dir);
        delete_dir_recursively(file_cache_tmp_dir);
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
