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

namespace {

const int kDirentsCacheExpireTime = 60 * 1000;

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

SINGLETON_IMPL(FileCacheDB)
FileCacheDB::FileCacheDB()
{
    db_ = NULL;
}

FileCacheDB::~FileCacheDB()
{
    if (db_ != NULL)
        sqlite3_close(db_);
}

void FileCacheDB::start()
{
    const char *errmsg;
    const char *sql;
    sqlite3 *db;

    QString db_path = QDir(seafApplet->configurator()->seafileDir()).filePath("file-cache.db");
    if (sqlite3_open (toCStr(db_path), &db)) {
        errmsg = sqlite3_errmsg (db);
        qDebug("failed to open file cache database %s: %s",
               toCStr(db_path), errmsg ? errmsg : "no error given");

        seafApplet->errorAndExit(QObject::tr("failed to open file cache database"));
        return;
    }

    sql = "DROP TABLE IF EXISTS FileCache";
    sqlite_query_exec (db, sql);

    sql = "CREATE TABLE IF NOT EXISTS FileCacheV1 ("
        "     repo_id VARCHAR(36), "
        "     path VARCHAR(4096), "
        "     file_id VARCHAR(40) NOT NULL, "
        "     account_sig VARCHAR(40) NOT NULL, "
        "     PRIMARY KEY (repo_id, path))";
    sqlite_query_exec (db, sql);

    db_ = db;
}

bool FileCacheDB::getCacheEntryCB(sqlite3_stmt *stmt, void *data)
{
    CacheEntry *entry = (CacheEntry *)data;
    entry->repo_id = (const char *)sqlite3_column_text (stmt, 0);
    entry->path = QString::fromUtf8((const char *)sqlite3_column_text (stmt, 1));
    entry->file_id = (const char *)sqlite3_column_text (stmt, 2);
    entry->account_sig = (const char *)sqlite3_column_text (stmt, 3);
    return true;
}

QString FileCacheDB::getCachedFileId(const QString& repo_id,
                                     const QString& path)
{
    return getCacheEntry(repo_id, path).file_id;
}

FileCacheDB::CacheEntry FileCacheDB::getCacheEntry(const QString& repo_id,
                                                   const QString& path)
{
    QString sql = "SELECT repo_id, path, file_id, account_sig"
        "  FROM FileCacheV1"
        "  WHERE repo_id = '%1'"
        "    AND path = '%2'";
    sql = sql.arg(repo_id).arg(path);
    CacheEntry entry;
    sqlite_foreach_selected_row (db_, toCStr(sql), getCacheEntryCB, &entry);
    return entry;
}

void FileCacheDB::saveCachedFileId(const QString& repo_id,
                                   const QString& path,
                                   const QString& file_id,
                                   const QString& account_sig)
{
    QString sql = "REPLACE INTO FileCacheV1 VALUES ('%1', '%2', '%3', '%4')";
    sql = sql.arg(repo_id).arg(path).arg(file_id).arg(account_sig);
    sqlite_query_exec (db_, toCStr(sql));
}

bool FileCacheDB::collectCachedFile(sqlite3_stmt *stmt, void *data)
{
    QList<CacheEntry> *list = (QList<CacheEntry> *)data;
    CacheEntry entry;
    entry.repo_id = (const char *)sqlite3_column_text (stmt, 0);
    entry.path = QString::fromUtf8((const char *)sqlite3_column_text (stmt, 1));
    entry.account_sig = (const char *)sqlite3_column_text (stmt, 2);
    list->append(entry);
    return true;
}

QList<FileCacheDB::CacheEntry> FileCacheDB::getAllCachedFiles()
{
    QString sql = "SELECT repo_id, path, account_sig FROM FileCacheV1";
    QList<CacheEntry> list;
    sqlite_foreach_selected_row (db_, toCStr(sql), collectCachedFile, &list);
    return list;
}
