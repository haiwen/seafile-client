#include <QDir>
#include <QFileInfo>

#include "seafile-applet.h"
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
            // The account might have been deleted
            printf ("account of file %s already deleted\n", toCStr(entry.path));
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
    if (!QFileInfo(local_path).exists()) {
        printf("cached file %s does not exist anymore\n",
               toCStr(local_path));
        return;
    }

    printf("watch local file %s\n", toCStr(local_path));
    watcher_.addPath(local_path);
    watch_infos_[local_path] = WatchedFileInfo(account, repo_id, path);
}

void AutoUpdateManager::onFileChanged(const QString& local_path)
{
    printf("detected file change: %s\n", toCStr(local_path));
    QString repo_id, path_in_repo;
    if (!watch_infos_.contains(local_path)) {
        return;
    }
    if (!QFileInfo(local_path).exists()) {
        printf("watched file no long exists: %s\n", toCStr(local_path));
        watch_infos_.remove(local_path);
        watcher_.removePath(local_path);
        return;
    }

    WatchedFileInfo& info = watch_infos_[local_path];

    printf("repo %s, path %s\n",
           toCStr(info.repo_id), toCStr(info.path_in_repo));
    LocalRepo repo;
    seafApplet->rpcClient()->getLocalRepo(info.repo_id, &repo);
    if (repo.isValid()) {
        printf ("repo %s already downloaded to local, no need to update\n",
                toCStr(repo.id));
        return;
    }

    DataManager data_mgr(info.account);
    FileNetworkTask *task = data_mgr.createUploadTask(
        info.repo_id, ::getParentPath(info.path_in_repo),
        local_path, ::getBaseName(local_path), true);

    connect(task, SIGNAL(finished(bool)),
            this, SLOT(onUpdateTaskFinished(bool)));

    task->start();
    info.uploading = true;
}

void AutoUpdateManager::onUpdateTaskFinished(bool success)
{
    FileUploadTask *task = qobject_cast<FileUploadTask *>(sender());
    if (task == NULL)
        return;
    const QString local_path = task->localFilePath();
    if (success) {
        printf("re-add watch for %s\n", toCStr(local_path));
        emit fileUpdated(task->repoId(), task->path());
    } else {
        qDebug("failed to auto update %s\n", toCStr(local_path));
    }
    watcher_.addPath(local_path);
    WatchedFileInfo& info = watch_infos_[local_path];
    info.uploading = false;
}
