#ifndef __UTILS_H_
#define __UTILS_H_

struct sqlite3;
struct sqlite3_stmt;

typedef bool (*SqliteRowFunc) (sqlite3_stmt *stmt, void *data);

sqlite3_stmt *sqlite_query_prepare (sqlite3 *db, const char *sql);

int sqlite_query_exec (sqlite3 *db, const char *sql);

int sqlite_foreach_selected_row (sqlite3 *db, const char *sql, 
                                 SqliteRowFunc callback, void *data);

int checkdir_with_mkdir (const char *dir);

#endif
