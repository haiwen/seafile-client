#include <sqlite3.h>
#include <glib.h>

#include "utils.h"

typedef bool (*SqliteRowFunc) (sqlite3_stmt *stmt, void *data);

sqlite3_stmt *
sqlite_query_prepare (sqlite3 *db, const char *sql)
{
    sqlite3_stmt *stmt;
    int result;

    result = sqlite3_prepare_v2 (db, sql, -1, &stmt, NULL);

    if (result != SQLITE_OK) {
        const gchar *str = sqlite3_errmsg (db);

        g_warning ("Couldn't prepare query, error:%d->'%s'\n\t%s\n", 
                   result, str ? str : "no error given", sql);

        return NULL;
    }

    return stmt;
}

int sqlite_query_exec (sqlite3 *db, const char *sql)
{
    char *errmsg = NULL;
    int result;

    result = sqlite3_exec (db, sql, NULL, NULL, &errmsg);

    if (result != SQLITE_OK) {
        if (errmsg != NULL) {
            g_warning ("SQL error: %d - %s\n:\t%s\n", result, errmsg, sql);
            sqlite3_free (errmsg);
        }
        return -1;
    }

    return 0;
}

int sqlite_foreach_selected_row (sqlite3 *db, const char *sql, 
                                 SqliteRowFunc callback, void *data)
{
    sqlite3_stmt *stmt;
    int result;
    int n_rows = 0;

    stmt = sqlite_query_prepare (db, sql);
    if (!stmt) {
        return -1;
    }

    while (1) {
        result = sqlite3_step (stmt);
        if (result != SQLITE_ROW)
            break;
        n_rows++;
        if (!callback (stmt, data))
            break;
    }

    if (result == SQLITE_ERROR) {
        const gchar *s = sqlite3_errmsg (db);

        g_warning ("Couldn't execute query, error: %d->'%s'\n",
                   result, s ? s : "no error given");
        sqlite3_finalize (stmt);
        return -1;
    }

    sqlite3_finalize (stmt);
    return n_rows;
}

int checkdir_with_mkdir (const char *dir)
{
#ifdef WIN32
    int ret;
    char *path = g_strdup(dir);
    char *p = (char *)path + strlen(path) - 1;
    while (*p == '\\' || *p == '/') *p-- = '\0';
    ret = g_mkdir_with_parents(path, 0755);
    g_free (path);
    return ret;
#else
    return g_mkdir_with_parents(dir, 0755);
#endif
}
