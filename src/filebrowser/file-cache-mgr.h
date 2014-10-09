#ifndef SEAFILE_CLIENT_FILE_BROWSER_FILE_CACHE_H
#define SEAFILE_CLIENT_FILE_BROWSER_FILE_CACHE_H

class CppSQLite3DB;
#include <QString>
/*
 * Singleton class for persistent file cache
 * Designed to be used by multiple instances of FileBrowserDiaglog and DataMgr
 * and of course, across different Accounts and ServerRepos
 * Exceptions should be caught by all, and methods are nothrow
 */
class FileCacheManager {
public:
    static FileCacheManager& getInstance() {
        static FileCacheManager instance;
        return instance;
    }
    void open();
    void close();
    QString get(const QString &oid);
    QString get(const QString &oid, const QString &parent_dir,
                const QString &repo_id);
    void set(const QString &oid, const QString &file_location,
             const QString &file_name, const QString &parent_dir = "/",
             const QString &account = QString(), const QString &repo_id = QString());

private:
    explicit FileCacheManager();
    ~FileCacheManager();
    explicit FileCacheManager(const FileCacheManager&) {}
    FileCacheManager& operator=(const FileCacheManager&) { return *this; }

private:
    void createTableIfNotExist();
    void remove(const QString &oid, const QString &parent_dir,
                const QString &repo_id);
    void remove(const QString &oid);
    bool enabled_;
    bool closed_;
    CppSQLite3DB *db_;
    QString db_path_;
};

#endif // SEAFILE_CLIENT_FILE_BROWSER_FILE_CACHE_H
