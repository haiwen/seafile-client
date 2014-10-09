#include "file-cache-mgr.h"

#include "CppSQLite3.h"
#include <cstdio>
#include <QDir>
#include <QDebug>

#include "configurator.h"
#include "seafile-applet.h"

enum {
    FILE_CACHE_ID_MAX = 40,
    FILE_CACHE_PATH_MAX = 4096
};


FileCacheManager::FileCacheManager()
  : enabled_(true), closed_(true), db_(new CppSQLite3DB)
{
    qDebug("[file cache] sqlite version %s loaded", db_->SQLiteVersion());
    db_path_ = QDir(seafApplet->configurator()->seafileDir()).absoluteFilePath("file-cache.db");
}

FileCacheManager::~FileCacheManager()
{
    try {
        close();
    } catch (CppSQLite3Exception &e) {
        qWarning("[file cache] %s", e.errorMessage());
    }
}

void FileCacheManager::open()
{
    if (!closed_)
        return;
    try {
        db_->open(db_path_.toUtf8().constData());
        createTableIfNotExist();
    } catch (CppSQLite3Exception &e) {
        qWarning("[file cache] %s", e.errorMessage());
        qWarning("[file cache] set to be disabled");
        enabled_ = false;
        return;
    }

}

void FileCacheManager::close()
{
    if (enabled_ && !closed_ )
        db_->close(); // no exception could be throw here
    delete db_;
}

void FileCacheManager::createTableIfNotExist()
{
    if (!enabled_)
        return;
    static const char sql_create_table[] =
      "CREATE TABLE IF NOT EXISTS FileCache "
      "(path VARCHAR(4097), "
      " repo_id VARCHAR(40), "
      " oid VARCHAR(40) NOT NULL,"
      " PRIMARY KEY(path, repo_id))";
    static const char sql_create_index[] =
      "CREATE INDEX IF NOT EXISTS repo_idx "
      "on FileCache (repo_id)";
    try {
        db_->execQuery(sql_create_table);
        db_->execQuery(sql_create_index);
    } catch (CppSQLite3Exception &e) {
        qWarning("[file cache] %s", e.errorMessage());
        return;
    }
}

QString FileCacheManager::get(const QString &path,
                              const QString &repo_id) const
{
    if (!enabled_ || path.size() > FILE_CACHE_PATH_MAX ||
        repo_id.size() > FILE_CACHE_ID_MAX)
        return QString();

    static const char sql[] =
      "SELECT oid FROM FileCache WHERE path='%1' AND repo_id='%2'";

    QString buf = QString(sql).arg(path).arg(repo_id);
    CppSQLite3Query q;
    try {
        q = db_->execQuery(buf.toUtf8().constData());
    } catch (CppSQLite3Exception &e) {
        qDebug("[file cache] %s", e.errorMessage());
        return QString();
    }

    if (q.eof() || q.numFields() < 1)
        return QString();

    return QString::fromUtf8(q.getStringField(0));
}

void FileCacheManager::set(const QString &path, const QString &repo_id,
                           const QString &oid)

{
    if (!enabled_ || path.size() > FILE_CACHE_PATH_MAX ||
        repo_id.size() > FILE_CACHE_ID_MAX ||
        oid.size() > FILE_CACHE_ID_MAX)
        return;
    if (oid.isEmpty())
        return;

    static const char sql[] = "INSERT OR REPLACE INTO "
      " FileCache VALUES ('%1', '%2', '%3') ";

    QString buf = QString(sql).arg(path).arg(repo_id).arg(oid);
    try {
        CppSQLite3Query q = db_->execQuery(buf.toUtf8().constData());
    } catch (CppSQLite3Exception &e) {
        qDebug("[file cache] %s", e.errorMessage());
        return;
    }
}

void FileCacheManager::remove(const QString &path,
                              const QString &repo_id)
{
    // not need to check since it is used internally
    //if (!enabled_ || path.size() > FILE_CACHE_PATH_MAX ||
    //    repo_id.size() > FILE_CACHE_ID_MAX)
    //    return;

    static const char sql[] =
      "DELETE FROM FileCache WHERE path='%1' AND repo_id='%2'";

    QString buf = QString(sql).arg(path).arg(repo_id);
    CppSQLite3Query q;
    try {
        q = db_->execQuery(buf.toUtf8().constData());
    } catch (CppSQLite3Exception &e) {
        qDebug("[file cache] %s", e.errorMessage());
        return;
    }
}

bool FileCacheManager::expectCachedInLocation(
    const QString &path,
    const QString &repo_id,
    const QString &expected_oid,
    const QString &expected_location)
{
    if (!expectCachedInLocationWithoutRemove(path, repo_id, expected_oid,
                                             expected_location)) {
      remove(path, repo_id);
      return false;
    }

    return true;
}

bool FileCacheManager::expectCachedInLocationWithoutRemove(
    const QString &path,
    const QString &repo_id,
    const QString &expected_oid,
    const QString &expected_location) const
{
    if (expected_location.isEmpty() ||
        !QFileInfo(expected_location).isFile())
        return false;

    if (expected_oid != get(path, repo_id))
        return false;

    return true;
}

