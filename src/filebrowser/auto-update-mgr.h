#ifndef SEAFILE_CLIENT_FILE_BROWSER_AUTO_UPDATE_MANAGER_H
#define SEAFILE_CLIENT_FILE_BROWSER_AUTO_UPDATE_MANAGER_H

#include <QFileSystemWatcher>
#include <QHash>
#include <QQueue>
#include <QRunnable>

#include "utils/singleton.h"
#include "account.h"
#include "data-cache.h"

struct FileTaskRecord;

class AutoUpdateManager : public QObject {
    SINGLETON_DEFINE(AutoUpdateManager)
    Q_OBJECT

public:
    void start();
    void removeWatch(const QString& path);
    void watchCachedFile(const Account& account,
                         const QString& repo_id,
                         const QString& path);
    void cleanCachedFile();

signals:
    void fileUpdated(const QString& repo_id, const QString& path);

private slots:
    void onFileChanged(const QString& path);
    void onUpdateTaskSuccess(const FileTaskRecord* task);
    void onUpdateTaskFailed(const FileTaskRecord* task);
    void checkFileRecreated();

private:
    AutoUpdateManager();

    QFileSystemWatcher watcher_;

    struct WatchedFileInfo {
        Account account;
        QString repo_id;
        QString path_in_repo;
        bool uploading;

        WatchedFileInfo() : uploading(false) {}
        WatchedFileInfo(const Account& account,
                        const QString& repo_id,
                        const QString& path_in_repo)
            : account(account),
              repo_id(repo_id),
              path_in_repo(path_in_repo),
              uploading(false) {}
    };

    QHash<QString, WatchedFileInfo> watch_infos_;

    QQueue<WatchedFileInfo> deleted_files_infos_;
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
