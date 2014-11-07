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

QList<SeafDirent>*
DirentsCache::getCachedDirents(const QString& repo_id,
                               const QString& path)
{
    QString cache_key = repo_id + path;
    CacheEntry *e = cache_->object(cache_key);
    if (e != NULL) {
        qint64 now = QDateTime::currentMSecsSinceEpoch();
        if (now < e->timestamp + kDirentsCacheExpireTime) {
            return &(e->dirents);
        }
    }

    return NULL;
}

void DirentsCache::expireCachedDirents(const QString& repo_id, const QString& path)
{
    cache_->remove(repo_id + path);
}

void DirentsCache::saveCachedDirents(const QString& repo_id,
                                     const QString& path,
                                     const QList<SeafDirent>& dirents)
{
    CacheEntry *val = new CacheEntry;
    val->timestamp = QDateTime::currentMSecsSinceEpoch();
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

    sql = "CREATE TABLE IF NOT EXISTS FileCache ("
        "     repo_id VARCHAR(36), "
        "     path VARCHAR(4096), "
        "     file_id VARCHAR(40) NOT NULL, "
        "     PRIMARY KEY (repo_id, path))";
    sqlite_query_exec (db, sql);

    db_ = db;
}

bool FileCacheDB::getCacheIdCB(sqlite3_stmt *stmt, void *data)
{
    QString *id = (QString *)data;
    const char *file_id = (const char *)sqlite3_column_text (stmt, 0);
    *id = file_id;
    return true;
}

QString FileCacheDB::getCachedFileId(const QString& repo_id,
                                     const QString& path)
{
    QString sql = "SELECT file_id"
        "  FROM FileCache"
        "  WHERE repo_id = '%1'"
        "    AND path = '%2'";
    sql = sql.arg(repo_id).arg(path);
    QString file_id;
    sqlite_foreach_selected_row (db_, toCStr(sql), getCacheIdCB, &file_id);
    return file_id;
}

void FileCacheDB::saveCachedFileId(const QString& repo_id,
                                   const QString& path,
                                   const QString& file_id)
{
    QString sql = "REPLACE INTO FileCache VALUES ('%1', '%2', '%3')";
    sql = sql.arg(repo_id).arg(path).arg(file_id);
    sqlite_query_exec (db_, toCStr(sql));
}
