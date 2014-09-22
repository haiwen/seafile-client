#ifndef SEAFILE_CLIENT_FILE_BROWSER_FILE_CACHE_H
#define SEAFILE_CLIENT_FILE_BROWSER_FILE_CACHE_H

class CppSQLite3DB;
#include <QString>
/*
 * Singleton class for persistent file cache
 * Designed to be used by multiple instances of FileBrowserDiaglog and DataMgr
 * and of course, across different Accounts and ServerRepos
 * Exceptions are used for control-flow
 */
class FileCacheManager {
public:
    inline static FileCacheManager& getInstance() {
        static FileCacheManager instance;
        return instance;
    }
    void open();
    void close();
    QString get(const QString &oid);
    void set(const QString &oid, const QString &file_location,
             const QString &file_name, const QString &parent_dir = "/",
             const QString &account = QString(), const QString &repo_id = QString());

private:
    explicit FileCacheManager();
    ~FileCacheManager();
    inline explicit FileCacheManager(const FileCacheManager&) {}
    inline FileCacheManager& operator=(const FileCacheManager&) { return *this; }
    void createTableIfNotExist();

private:
    bool enabled_;
    bool closed_;
    void remove(const QString &oid);
    CppSQLite3DB *db_;
    QString db_path_;
};

#endif // SEAFILE_CLIENT_FILE_BROWSER_FILE_CACHE_H
