#ifndef SEAFILE_CLIENT_FILE_BROWSER_AUTO_UPDATE_MANAGER_H
#define SEAFILE_CLIENT_FILE_BROWSER_AUTO_UPDATE_MANAGER_H

#include <QFileSystemWatcher>
#include <QHash>
#include <QQueue>
#include <QRunnable>
#include <QScopedPointer>

#include "utils/singleton.h"
#include "account.h"
#include "data-cache.h"
#include "data-mgr.h"

class AutoUpdateManager : public QObject {
    SINGLETON_DEFINE(AutoUpdateManager)
    Q_OBJECT

public:
    void start();
    void removeWatch(const QString& path);
    void watchCachedFile(const Account& account,
                         const QString& repo_id,
                         const QString& path);

    enum FileStatus {
        // Local cache is consistent with the version on the server
        SYNCED = 0,
        // The file is being auto-uploaded
        UPLOADING,
        // Not synced and also not uploading, this happens e.g. when the upload
        // request failed
        NOT_SYNCED,
    };

    QHash<QString, FileStatus> getFileStatusForDirectory(
        const QString &account_sig,
        const QString &repo_id,
        const QString &path,
        const QList<SeafDirent>& dirents);
    void cleanCachedFile();
    void uploadFile(const QString& local_path);
    void dumpCacheStatus();

signals:
    void fileUpdated(const QString& repo_id, const QString& path);

private slots:
    void onFileChanged(const QString& path);
    void onUpdateTaskFinished(bool success);
    void checkFileRecreated();
    void systemShutDown();

private:
    AutoUpdateManager();

    QFileSystemWatcher watcher_;

    struct WatchedFileInfo {
        Account account;
        QString repo_id;
        QString path_in_repo;
        qint64 mtime;
        qint64 fsize;

        bool uploading;

        WatchedFileInfo() {}
        WatchedFileInfo(const Account& account,
                        const QString& repo_id,
                        const QString& path_in_repo,
                        qint64 mtime,
                        qint64 fsize)
            : account(account),
              repo_id(repo_id),
              path_in_repo(path_in_repo),
              mtime(mtime),
              fsize(fsize)
              {}
    };

    QHash<QString, WatchedFileInfo> watch_infos_;

    QQueue<WatchedFileInfo> deleted_files_infos_;

    bool system_shut_down_;
};

#ifdef Q_OS_MAC
/**
 * On MacOSX, when open an image file in Preview app, a file modificatin event
 * would be triggered, but the file is not modified. We need to work around
 * this so the auto update manager would not be fooled by this false signal.
 */
class MacImageFilesWorkAround {
    SINGLETON_DEFINE(MacImageFilesWorkAround)
public:
    MacImageFilesWorkAround();
    bool isRecentOpenedImage(const QString& path);
    void fileOpened(const QString& path);

private:
    QHash<QString, qint64> images_;
};
#endif // Q_OS_MAC

class CachedFilesCleaner : public QObject, public QRunnable {
    Q_OBJECT
public:
    CachedFilesCleaner();
    void run();
    bool autoDelete() {
        return true;
    }
};

#endif // SEAFILE_CLIENT_FILE_BROWSER_AUTO_UPDATE_MANAGER_H
