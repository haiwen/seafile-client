#ifndef SEAFILE_CLIENT_UTILS_H_
#define SEAFILE_CLIENT_UTILS_H_

#if defined(XCODE_APP)
#define RESOURCE_PATH(_name)  (QDir(QCoreApplication::applicationDirPath()).filePath(QString("../Resources/")+(_name)))
#else
#define RESOURCE_PATH(_name)  (QDir::current().filePath(_name))
#endif

struct sqlite3;
struct sqlite3_stmt;

#define toCStr(_s)   ((_s).isNull() ? NULL : (_s).toUtf8().data())

typedef bool (*SqliteRowFunc) (sqlite3_stmt *stmt, void *data);

sqlite3_stmt *sqlite_query_prepare (sqlite3 *db, const char *sql);

int sqlite_query_exec (sqlite3 *db, const char *sql);

int sqlite_foreach_selected_row (sqlite3 *db, const char *sql, 
                                 SqliteRowFunc callback, void *data);

int checkdir_with_mkdir (const char *dir);

int process_is_running (const char *process_name);

void shutdown_process (const char *name);


void open_dir(const QString& path);

int get_seafile_auto_start();

int set_seafile_auto_start(int on);

typedef bool (*KeyValueFunc) (void *data, const char *key,
                              const char *value);

bool parse_key_value_pairs (char *string, KeyValueFunc func, void *data);


#endif
