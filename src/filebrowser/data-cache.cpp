#include <QDir>
#include <sqlite3.h>
#include <errno.h>
#include <stdio.h>

#include <QDateTime>
#include <QCache>

#include "utils/file-utils.h"
#include "utils/utils.h"
#include "configurator.h"
#include "seafile-applet.h"
#include "data-cache.h"
#include "data-mgr.h"
#include "account-mgr.h"

namespace {

const int kDirentsCacheExpireTime = 60 * 1000;

void filecache_entry_from_sqlite3_result(sqlite3_stmt *stmt, FileCache::CacheEntry* entry)
{
    entry->repo_id = (const char *)sqlite3_column_text (stmt, 0);
    entry->path = QString::fromUtf8((const char *)sqlite3_column_text (stmt, 1));
    entry->account_sig = (const char *)sqlite3_column_text (stmt, 2);
    entry->file_id = (const char *)sqlite3_column_text (stmt, 3);
    entry->seafile_mtime = sqlite3_column_int64 (stmt, 4);
    entry->seafile_size = sqlite3_column_int64 (stmt, 5);
}

} // namespace

SINGLETON_IMPL(DirentsCache)
DirentsCache::DirentsCache()
{
    cache_ = new QCache<QString, CacheEntry>;
}
DirentsCache::~DirentsCache()
{
    delete cache_;
}

DirentsCache::ReturnEntry
DirentsCache::getCachedDirents(const QString& repo_id,
                               const QString& path)
{
    QString cache_key = repo_id + path;
    CacheEntry *e = cache_->object(cache_key);
    if (e != NULL) {
        qint64 now = QDateTime::currentMSecsSinceEpoch();
        if (now < e->timestamp + kDirentsCacheExpireTime) {
            return ReturnEntry(e->current_readonly, &(e->dirents));
        }
    }

    return ReturnEntry(false, NULL);
}

void DirentsCache::expireCachedDirents(const QString& repo_id, const QString& path)
{
    cache_->remove(repo_id + path);
}

void DirentsCache::saveCachedDirents(const QString& repo_id,
                                     const QString& path,
                                     bool current_readonly,
                                     const QList<SeafDirent>& dirents)
{
    CacheEntry *val = new CacheEntry;
    val->timestamp = QDateTime::currentMSecsSinceEpoch();
    val->current_readonly = current_readonly;
    val->dirents = dirents;
    QString cache_key = repo_id + path;
    cache_->insert(cache_key, val);
}

#if 0
SINGLETON_IMPL(FileCache)
FileCache::FileCache()
{
    cache_ = new QHash<QString, CacheEntry>;
}

FileCache::~FileCache()
{
    delete cache_;
}

QString FileCache::getCachedFileId(const QString& repo_id,
                                     const QString& path)
{
    return getCacheEntry(repo_id, path).file_id;
}

FileCache::CacheEntry FileCache::getCacheEntry(const QString& repo_id,
                                                   const QString& path)
{
    QString cache_key = repo_id + path;

    return cache_->value(cache_key);
}

void FileCache::saveCachedFileId(const QString& repo_id,
                                   const QString& path,
                                   const QString& file_id,
                                   const QString& account_sig)
{
    CacheEntry val;
    QString cache_key = repo_id + path;

    val.repo_id = repo_id;
    val.path = path;
    val.file_id = file_id;
    val.account_sig = account_sig;

    cache_->insert(cache_key, val);
}

QList<FileCache::CacheEntry> FileCache::getAllCachedFiles()
{
    return cache_->values();
}

void FileCache::cleanCurrentAccountCache()
{
    const Account cur_account = seafApplet->accountManager()->currentAccount();
    foreach(const QString& key, cache_->keys()) {
        const Account account = seafApplet->accountManager()
                                ->getAccountBySignature(cache_->value(key).account_sig);
        if (account == cur_account)
            cache_->remove(key);
    }
}
#endif

SINGLETON_IMPL(FileCache)
FileCache::FileCache()
{
    db_ = NULL;
}

FileCache::~FileCache()
{
    if (db_ != NULL)
        sqlite3_close(db_);
}

void FileCache::start()
{
    const char *errmsg;
    const char *sql;
    sqlite3 *db;

    QString db_path = QDir(seafApplet->configurator()->seafileDir()).filePath("autoupdate-cache.db");
    if (sqlite3_open (toCStr(db_path), &db)) {
        errmsg = sqlite3_errmsg (db);
        qDebug("failed to open file cache database %s: %s",
               toCStr(db_path), errmsg ? errmsg : "no error given");

        seafApplet->errorAndExit(QObject::tr("failed to open file cache database"));
        return;
    }

    // Drop the old table.
    // XX(lins05): This is not ideal. Should we invent a table schema upgrade mechanism?
    sql = "DROP TABLE IF EXISTS FileCache;";
    sqlite_query_exec (db, sql);
    sql = "DROP TABLE IF EXISTS FileCacheV1;";
    sqlite_query_exec (db, sql);

    sql = "CREATE TABLE IF NOT EXISTS FileCacheV2 ("
        "     repo_id VARCHAR(36), "
        "     path VARCHAR(4096), "
        "     account_sig VARCHAR(40) NOT NULL, "
        "     file_id VARCHAR(40) NOT NULL, "
        "     seafile_mtime integer NOT NULL, "
        "     seafile_size integer NOT NULL, "
        "     PRIMARY KEY (repo_id, path))";
    sqlite_query_exec (db, sql);

    db_ = db;
}

bool FileCache::getCacheEntryCB(sqlite3_stmt *stmt, void *data)
{
    CacheEntry *entry = (CacheEntry *)data;
    filecache_entry_from_sqlite3_result(stmt, entry);
    return true;
}

bool FileCache::getCacheEntry(const QString& repo_id,
                              const QString& path,
                              FileCache::CacheEntry *entry)
{
    char *zql = sqlite3_mprintf("SELECT *"
                                "  FROM FileCacheV2"
                                " WHERE repo_id = %Q"
                                "   AND path = %Q",
                                repo_id.toUtf8().data(), path.toUtf8().data());
    bool ret = sqlite_foreach_selected_row(db_, zql, getCacheEntryCB, entry) > 0;
    sqlite3_free(zql);
    return ret;
}

void FileCache::saveCachedFileId(const QString& repo_id,
                                 const QString& path,
                                 const QString& account_sig,
                                 const QString& file_id,
                                 const QString& local_file_path)
{
    QFileInfo finfo (local_file_path);
    qint64 mtime = finfo.lastModified().toMSecsSinceEpoch();
    qint64 fsize = finfo.size();
    char *zql = sqlite3_mprintf("REPLACE INTO FileCacheV2( "
                                "repo_id, path, account_sig, file_id, "
                                "seafile_mtime, seafile_size "
                                ") VALUES (%Q, %Q, %Q, %Q, %lld, %lld)",
                                toCStr(repo_id),
                                toCStr(path),
                                toCStr(account_sig),
                                toCStr(file_id),
                                mtime,
                                fsize);
    sqlite_query_exec(db_, zql);
    sqlite3_free(zql);
}

bool FileCache::collectCachedFile(sqlite3_stmt *stmt, void *data)
{
    QList<CacheEntry> *list = (QList<CacheEntry> *)data;

    CacheEntry entry;
    filecache_entry_from_sqlite3_result(stmt, &entry);
    list->append(entry);
    return true;
}

QList<FileCache::CacheEntry> FileCache::getAllCachedFiles()
{
    const char* sql = "SELECT * FROM FileCacheV2";
    QList<CacheEntry> list;
    sqlite_foreach_selected_row(db_, sql, collectCachedFile, &list);
    return list;
}

void FileCache::cleanCurrentAccountCache()
{
    const Account account = seafApplet->accountManager()->currentAccount();
    if (!account.isValid()) {
        return;
    }
    char *zql = sqlite3_mprintf(
        "DELETE FROM FileCacheV2 where account_sig = %Q",
        toCStr(account.getSignature()));
    sqlite_query_exec(db_, zql);
    sqlite3_free(zql);
}

QList<FileCache::CacheEntry> FileCache::getCachedFilesForDirectory(const QString& account_sig,
                                                                   const QString& repo_id,
                                                                   const QString& parent_dir_in)
{
    QString parent_dir = parent_dir_in;
    // Strip the trailing slash
    if (parent_dir.length() > 1 && parent_dir.endsWith("/")) {
        parent_dir = parent_dir.left(parent_dir.length() - 1);
    }

    QList<CacheEntry> entries;
    char* sql = sqlite3_mprintf("SELECT * FROM FileCacheV2 "
                                "WHERE repo_id = %Q "
                                "  AND path like %Q "
                                "  AND account_sig = %Q; ",
                                toCStr(repo_id),
                                toCStr(QString("%1%").arg(parent_dir == "/" ? parent_dir : parent_dir + "/")),
                                toCStr(account_sig));

    sqlite_foreach_selected_row(db_, sql, collectCachedFile, &entries);

    // Even if we filtered the path in the above sql query, the returned entries
    // may still belong to subdirectory of "parent_dir" instead of parent_dir
    // itself. So we need to fitler again.
    QList<CacheEntry> ret;
    foreach(const CacheEntry& entry, entries) {
        if (::getParentPath(entry.path) == parent_dir) {
            QString localpath = DataManager::instance()->getLocalCacheFilePath(entry.repo_id, entry.path);
            if (QFileInfo(localpath).exists()) {
                ret.append(entry);
            }
        }
    }
    return ret;
}
