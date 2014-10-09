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
    void set(const QString &path, const QString &repo_id,
             const QString &oid);

    // Internally, first check if the file exists
    // then if the oids are matched
    bool expectCachedInLocation(
        const QString &path, const QString &repo_id,
        const QString &expected_oid, const QString &expected_location);

    // Internally, first check if the file exists
    // then if the oids are matched while not modifying anything
    bool expectCachedInLocationWithoutRemove(
        const QString &path, const QString &repo_id,
        const QString &expected_oid, const QString &expected_location) const;

private:
    explicit FileCacheManager();
    ~FileCacheManager();
    explicit FileCacheManager(const FileCacheManager&) {}
    FileCacheManager& operator=(const FileCacheManager&) { return *this; }

private:
    QString get(const QString &path, const QString &repo_id) const;
    void remove(const QString &path, const QString &repo_id);
    void createTableIfNotExist();
    bool enabled_;
    bool closed_;
    CppSQLite3DB *db_;
    QString db_path_;
};


#endif // SEAFILE_CLIENT_FILE_BROWSER_FILE_CACHE_H
