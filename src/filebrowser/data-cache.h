#ifndef SEAFILE_CLIENT_FILE_BROWSER_DATA_CACHE_H
#define SEAFILE_CLIENT_FILE_BROWSER_DATA_CACHE_H

#include <QList>
#include <utility>

#include "seaf-dirent.h"
#include "utils/singleton.h"

template<typename Key, typename T> class QCache;

struct sqlite3;
struct sqlite3_stmt;

/**
 * Cache dirents by (repo_id + path, dirents) in memory
 */
class DirentsCache {
    SINGLETON_DEFINE(DirentsCache)
public:
    typedef std::pair<bool ,QList<SeafDirent>*> ReturnEntry;
    ReturnEntry getCachedDirents(const QString& repo_id,
                                 const QString& path);

    void expireCachedDirents(const QString& repo_id, const QString& path);

    void saveCachedDirents(const QString& repo_id,
                           const QString& path,
                           bool current_readonly,
                           const QList<SeafDirent>& dirents);

private:
    DirentsCache();
    ~DirentsCache();
    struct CacheEntry {
        qint64 timestamp;
        bool current_readonly;
        QList<SeafDirent> dirents;
    };

    QCache<QString, CacheEntry> *cache_;
};

#if 0
/**
 * Record the file id of downloaded files.
 * The schema is (repo_id, path, downloaded_file_id)
 */
class FileCache {
    SINGLETON_DEFINE(FileCache)
public:
    struct CacheEntry {
        QString repo_id;
        QString path;
        QString file_id;
        QString account_sig;
    };


    QString getCachedFileId(const QString& repo_id,
                            const QString& path);
    CacheEntry getCacheEntry(const QString& repo_id,
                             const QString& path);
    void saveCachedFileId(const QString& repo_id,
                          const QString& path,
                          const QString& file_id,
                          const QString& account_sig);

    QList<CacheEntry> getAllCachedFiles();
    void cleanCurrentAccountCache();

private:
    FileCache();
    ~FileCache();

    QHash<QString, CacheEntry> *cache_;
};
#endif

/**
 * Record the file id of downloaded files.
 */
class FileCache {
    SINGLETON_DEFINE(FileCache)
public:
    struct CacheEntry {
        QString repo_id;
        QString path;
        QString account_sig;
        QString file_id;
        qint64  seafile_mtime;
        qint64  seafile_size;
    };

    void start();

    bool getCacheEntry(const QString& repo_id,
                       const QString& path,
                       CacheEntry *entry);
    void saveCachedFileId(const QString& repo_id,
                          const QString& path,
                          const QString& file_id,
                          const QString& account_sig,
                          const QString& local_file_path);

    QList<CacheEntry> getCachedFilesForDirectory(const QString& account_sig,
                                                 const QString& repo_id,
                                                 const QString& parent_dir);

    QList<CacheEntry> getAllCachedFiles();
    void cleanCurrentAccountCache();

private:
    FileCache();
    ~FileCache();
    static bool getCacheEntryCB(sqlite3_stmt *stmt, void *data);
    static bool collectCachedFile(sqlite3_stmt *stmt, void *data);

    sqlite3 *db_;
};


#endif // SEAFILE_CLIENT_FILE_BROWSER_DATA_CACHE_H
