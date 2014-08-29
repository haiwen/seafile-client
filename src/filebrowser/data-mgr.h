#ifndef SEAFILE_CLIENT_FILE_BROWSER_DATA_MANAGER_H
#define SEAFILE_CLIENT_FILE_BROWSER_DATA_MANAGER_H

#include <vector>
#include <QPair>
#include <QObject>

#include "api/api-error.h"
#include "account.h"
#include "seaf-dirent.h"

class GetDirentsRequest;
class FileBrowserCache;

/**
 * DataManager is responsible for getting dirents/files from seahub, as well
 * as the caching policy.
 *
 */
class DataManager : public QObject {
    Q_OBJECT
public:
    DataManager(const Account& account);

    void getDirents(const QString repo_id,
                    const QString& path);

signals:
    void getDirentsSuccess(const std::vector<SeafDirent>& dirents);
    void getDirentsFailed(const ApiError& error);

private slots:
    void onGetDirentsSuccess(const QString& dir_id,
                             const std::vector<SeafDirent>& dirents);
    // void onGetDirentsFailed(const ApiError& error);

private:
    Account account_;

    GetDirentsRequest *req_;

    FileBrowserCache *cache_;
};

// Manages cached dirents
class FileBrowserCache {
public:
    FileBrowserCache();
    QPair<QString, std::vector<SeafDirent> > getCachedDirents(const QString repo_id,
                                                              const QString& path);

    void saveDirents(const QString repo_id,
                     const QString& path,
                     const QString& dir_id,
                     const std::vector<SeafDirent>& dirents);

private:
    QString cache_dir_;
};

#endif // SEAFILE_CLIENT_FILE_BROWSER_DATA_MANAGER_H
