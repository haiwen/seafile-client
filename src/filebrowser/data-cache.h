#ifndef SEAFILE_CLIENT_FILE_BROWSER_DATA_CACHE_H
#define SEAFILE_CLIENT_FILE_BROWSER_DATA_CACHE_H

#include <QList>

#include "seaf-dirent.h"
#include "utils/singleton.h"

template<typename Key, typename T> class QCache;

/**
 * Cache dirents by (repo_id + path, dirents) in memory
 */
class DirentsCache {
    SINGLETON_DEFINE(DirentsCache)
public:
    QList<SeafDirent> *getCachedDirents(const QString& repo_id,
                                        const QString& path);

    void saveCachedDirents(const QString& repo_id,
                           const QString& path,
                           const QList<SeafDirent>& dirents);

private:
    DirentsCache();
    struct CacheEntry {
        qint64 timestamp;
        QList<SeafDirent> dirents;
    };

    QCache<QString, CacheEntry> *cache_;
};

/**
 * Record the file id of downloaded files.
 * The schema is (repo_id, path, downloaded_file_id)
 */
class FileCacheDB {
    SINGLETON_DEFINE(FileCacheDB)
public:
    void start();
    QString getCachedFileId(const QString& repo_id,
                            const QString& path);
    void saveCachedFileId(const QString& repo_id,
                          const QString& path,
                          const QString& file_id);
private:
    FileCacheDB();
    static bool getCacheIdCB(sqlite3_stmt *stmt, void *data);

    sqlite3 *db_;
};


#endif // SEAFILE_CLIENT_FILE_BROWSER_DATA_CACHE_H
