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
class FileCacheDB {
public:
    static FileCacheDB& getInstance() {
        static FileCacheDB instance;
        return instance;
    }
    void open();
    void close();
    void set(const QString &path,
             const QString &repo_id,
             const QString &oid);

    QString get(const QString &path,
                const QString &repo_id) const;

private:
    explicit FileCacheDB();
    ~FileCacheDB();
    explicit FileCacheDB(const FileCacheDB&) {}
    FileCacheDB& operator=(const FileCacheDB&) { return *this; }

private:
    void remove(const QString &path, const QString &repo_id);
    void createTableIfNotExist();
    bool enabled_;
    bool closed_;
    CppSQLite3DB *db_;
    QString db_path_;
};


#endif // SEAFILE_CLIENT_FILE_BROWSER_FILE_CACHE_H
