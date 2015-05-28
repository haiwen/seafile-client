#include <QDir>
#include <QFileInfo>
#include <QDateTime>
#include <QTimer>

#include "seafile-applet.h"
#include "ui/tray-icon.h"
#include "configurator.h"
#include "account-mgr.h"
#include "rpc/rpc-client.h"
#include "rpc/local-repo.h"
#include "utils/file-utils.h"
#include "utils/utils.h"
#include "data-mgr.h"
#include "tasks.h"

#include "auto-update-mgr.h"

namespace {

const char *kRelayAddressProperty = "relay-address";
const char *kEmailProperty = "email";
const char *kFileCacheTopDirName = "file-cache";

}

SINGLETON_IMPL(AutoUpdateManager)


AutoUpdateManager::AutoUpdateManager()
{
    connect(&watcher_, SIGNAL(fileChanged(const QString&)),
            this, SLOT(onFileChanged(const QString&)));
}

void AutoUpdateManager::start()
{
    QList<FileCacheDB::CacheEntry> all_files =
        FileCacheDB::instance()->getAllCachedFiles();
    foreach (const FileCacheDB::CacheEntry entry, all_files) {
        Account account = seafApplet->accountManager()->getAccountBySignature(
            entry.account_sig);
        if (!account.isValid()) {
            return;
        }
        watchCachedFile(account, entry.repo_id, entry.path);
    }
}

void AutoUpdateManager::watchCachedFile(const Account& account,
                                        const QString& repo_id,
                                        const QString& path)
{
    QString local_path = DataManager::getLocalCacheFilePath(repo_id, path);
    qDebug("added watch of: %s\n", toCStr(path));
    if (!QFileInfo(local_path).exists()) {
        qDebug("but it does not exist!\n");
        return;
    }

    watcher_.addPath(local_path);
    watch_infos_[local_path] = WatchedFileInfo(account, repo_id, path);
}

void AutoUpdateManager::onFileChanged(const QString& local_path)
{
    qDebug("file changed: %s\n", toCStr(local_path));
#ifdef Q_OS_MAC
    if (MacImageFilesWorkAround::instance()->isRecentOpenedImage(local_path)) {
        return;
    }
#endif
    watcher_.removePath(local_path);
    QString repo_id, path_in_repo;
    if (!watch_infos_.contains(local_path)) {
        qDebug("but not info for it watch_infos_\n");
        return;
    }

    WatchedFileInfo info = watch_infos_[local_path];

    if (!QFileInfo(local_path).exists()) {
        qDebug("but file deleted \n");
        removeWatch(local_path);
        // Some application would deleted and recreate the file when saving.
        // We work around that by double checking whether the file gets
        // recreated after a short period
        QTimer::singleShot(5000, this, SLOT(checkFileRecreated()));
        deleted_files_infos_.enqueue(info);
        return;
    }

    LocalRepo repo;
    seafApplet->rpcClient()->getLocalRepo(info.repo_id, &repo);
    if (repo.isValid()) {
        qDebug("but repo invalid\n");
        return;
    }

    DataManager data_mgr(info.account);
    FileNetworkTask *task = data_mgr.createUploadTask(
        info.repo_id, ::getParentPath(info.path_in_repo),
        local_path, ::getBaseName(local_path), true);

    connect(task, SIGNAL(finished(bool)),
            this, SLOT(onUpdateTaskFinished(bool)));

    qDebug("started upload task\n");

    task->start();
    info.uploading = true;
}

void AutoUpdateManager::onUpdateTaskFinished(bool success)
{
    FileUploadTask *task = qobject_cast<FileUploadTask *>(sender());
    if (task == NULL) {
        qDebug("task finished but is null");
        return;
    }
    const QString local_path = task->localFilePath();
    if (success) {
        qDebug("uploaded file %s successfully", toCStr(local_path));
        seafApplet->trayIcon()->showMessageWithRepo(task->repoId(),
                                                    tr("Upload Success"),
                                                    tr("File \"%1\"\nuploaded successfully.").arg(QFileInfo(local_path).fileName()));
        emit fileUpdated(task->repoId(), task->path());
        watcher_.addPath(local_path);
        WatchedFileInfo& info = watch_infos_[local_path];
        info.uploading = false;
    } else {
        seafApplet->trayIcon()->showMessageWithRepo(task->repoId(),
                                                    tr("Upload Failure"),
                                                    tr("File \"%1\"\nfailed to upload.").arg(QFileInfo(local_path).fileName()));
        qWarning("failed to auto update %s\n", toCStr(local_path));
        watch_infos_.remove(local_path);
        return;
    }
}

void AutoUpdateManager::removeWatch(const QString& path)
{
    watcher_.removePath(path);
    watch_infos_.remove(path);
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
        qDebug("file %s recreated", toCStr(info.path_in_repo));
        watcher_.addPath(path);
        watch_infos_[path] = info;
    }
}

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
