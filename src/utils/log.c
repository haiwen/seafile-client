#include <errno.h>
#include <time.h>
#include <glib/gstdio.h>

#include "log.h"

static FILE *logfp;

static GLogLevelFlags applet_log_level;

static int
checkdir_with_mkdir (const char *dir)
{
#if defined(_WIN32)
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

static void
applet_log (const gchar *log_domain, GLogLevelFlags log_level,
            const gchar *message, gpointer user_data)
{
#define BUFSIZE 1024
    if (log_level > applet_log_level || message == NULL)
        return;

    if (log_level & G_LOG_FLAG_FATAL)
        fputs (message, stderr);

    time_t t;
    struct tm *tm;
    char buf[BUFSIZE];
    size_t len;

    t = time(NULL);
    tm = localtime(&t);
    len = strftime (buf, BUFSIZE, "[%x %X]", tm);
    g_return_if_fail (len < BUFSIZE);
    fputs (buf, logfp);
    fputs (message, logfp);

    if (strlen(message) > 0 && message[strlen(message) - 1] != '\n') {
        fputs("\n", logfp);
    }
    fflush (logfp);
#undef BUFSIZE
}

static void
delete_large_log_file(const char* file)
{
    GStatBuf log_file_stat_buf;
    if (g_stat(file, &log_file_stat_buf) != 0) {
        // Do not warn if errno=2 (file not exist), because during
        // first run of the client the log files may not be created
        // yet.
        if (errno != 2) {
            g_warning ("Get log file %s stat failed errno=%d.", file, errno);
        }
        return;
    }

    const int delete_threshold = 300 * 1000 * 1000;
    if (log_file_stat_buf.st_size <= delete_threshold) {
        return;
    } else {
        const char* backup_file_name_postfix = "-old";
        GString *backup_file = g_string_new(file);
        g_string_insert(backup_file, backup_file->len - 4, backup_file_name_postfix);
        // 4 is length of log file postfix ".log"
        // rename log file "***.log" to "***-old.log"
        char file_name[4096] = {0};
        memcpy(file_name, backup_file->str, backup_file->len);
        if (backup_file) {
            g_string_free(backup_file, TRUE);
        }

        if (g_file_test(file_name, G_FILE_TEST_EXISTS)) {
            if (g_remove(file_name) != 0) {
                g_warning ("Delete old log file %s failed errno=%d.", file_name, errno);
                return;
            } else {
                g_warning ("Deleted old log file %s.", file_name);
            }
        }

        if (g_rename(file, file_name) == 0) {
            g_warning ("Renamed %s to backup file %s.", file, file_name);
            return;
        } else {
            g_warning ("Rename %s to backup file failed errno=%d.", file, errno);
            return;
        }
    }
}

int
applet_log_init (const char *ccnet_dir)
{
    char *logdir = g_build_filename (ccnet_dir, "logs", NULL);
    char *applet_log_file = g_build_filename(logdir, "applet.log", NULL);
    char *seafile_log_file = g_build_filename(logdir, "seafile.log", NULL);

    if (checkdir_with_mkdir (logdir) < 0) {
        g_free (logdir);
        return -1;
    }

    g_free (logdir);

    delete_large_log_file(applet_log_file);
    delete_large_log_file(seafile_log_file);

    /* record all log message */
    applet_log_level = G_LOG_LEVEL_DEBUG;

    if ((logfp = g_fopen (applet_log_file, "a+")) == NULL) {
        g_warning ("Open file %s failed errno=%d\n", applet_log_file, errno);
        g_free (applet_log_file);
        return -1;
    }

    g_log_set_handler (NULL, G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL
                       | G_LOG_FLAG_RECURSION, applet_log, NULL);

    g_log_set_handler ("Ccnet", G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL
                       | G_LOG_FLAG_RECURSION, applet_log, NULL);

    g_free (applet_log_file);

    return 0;
}
