#include <errno.h>
#include <time.h>
#include <glib/gstdio.h>

#if defined(_MSC_VER)
#include <windows.h>
#endif

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
    if (logfp != NULL) {
        fputs (buf, logfp);
        fputs (message, logfp);

        if (strlen(message) > 0 && message[strlen(message) - 1] != '\n') {
            fputs("\n", logfp);
        }
        fflush (logfp);
    } else {
        printf("%s %s", buf, message);
    }
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
    }

    const char* backup_file_name_postfix = "-old";
    GString *backup_file = g_string_new(file);
    g_string_insert(backup_file, backup_file->len - 4, backup_file_name_postfix);
    // 4 is length of log file postfix ".log"
    // rename log file "***.log" to "***-old.log"

    if (g_file_test(backup_file->str, G_FILE_TEST_EXISTS)) {
        if (g_remove(backup_file->str) != 0) {
            g_warning ("Delete old log file %s failed errno=%d.", backup_file->str, errno);
            g_string_free(backup_file, TRUE);
            return;
        } else {
            g_warning ("Deleted old log file %s.", backup_file->str);
        }
    }

    if (g_rename(file, backup_file->str) == 0) {
        g_warning ("Renamed %s to backup file %s.", file, backup_file->str);
    } else {
        g_warning ("Rename %s to backup file failed errno=%d.", file, errno);
    }
    g_string_free(backup_file, TRUE);
}

int
applet_log_init (const char *ccnet_dir)
{
    char *logdir = g_build_filename (ccnet_dir, "logs", NULL);
    char *applet_log_file = g_build_filename(logdir, "applet.log", NULL);

    if (checkdir_with_mkdir (logdir) < 0) {
        g_free (logdir);
        return -1;
    }

    g_free (logdir);

    /* record all log message */
    applet_log_level = G_LOG_LEVEL_DEBUG;

    if ((logfp = g_fopen (applet_log_file, "a+")) == NULL) {
        g_warning ("Open file %s failed errno=%d\n", applet_log_file, errno);
        g_free (applet_log_file);
        return -1;
    }

#if defined(_MSC_VER)
    // Avoid having the seafile daemon inherit the log file handle, which prevents file handle being opened by seafile daamon when gui rotates the log.
    intptr_t fd = _fileno(logfp);
    HANDLE h = (HANDLE)_get_osfhandle(fd);
    SetHandleInformation(h, HANDLE_FLAG_INHERIT, 0);
#endif

    g_log_set_handler (NULL, G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL
                       | G_LOG_FLAG_RECURSION, applet_log, NULL);

    g_log_set_handler ("Ccnet", G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL
                       | G_LOG_FLAG_RECURSION, applet_log, NULL);

    g_free (applet_log_file);

    return 0;
}

const int
seafile_log_reopen (const char *logfile, const char *logfile_old)
{
    FILE *fp;

    if (g_file_test(logfile_old, G_FILE_TEST_EXISTS)) {
        if (g_remove(logfile_old) != 0) {
            g_warning ("Delete old log file %s failed errno=%d.", logfile_old, errno);
            return -1;
        } else {
            g_warning ("Deleted old log file %s.", logfile_old);
        }
    }

    if (fclose(logfp) < 0) {
        g_warning ("Failed to close file %s\n", logfile);
        return -1;
    }
    logfp = NULL;

    if (g_rename(logfile, logfile_old) == 0) {
        g_warning ("Renamed %s to backup file %s.\n", logfile, logfile_old);
    } else {
        g_warning ("Rename %s to backup file failed errno=%d.\n", logfile, errno);
        return -1;
    }

#if defined(_MSC_VER)
    if ((fp = (FILE *)g_fopen (logfile, "a+")) == NULL) {
#else
    if ((fp = (FILE *)(long)g_fopen (logfile, "a+")) == NULL) {
#endif
        g_warning ("Failed to open file %s\n", logfile);
        return -1;
    }
    logfp = fp;

    return 0;
}

void
applet_log_rotate (const char *ccnet_dir)
{
    char *logdir = g_build_filename (ccnet_dir, "logs", NULL);
    char *applet_log_file = g_build_filename(logdir, "applet.log", NULL);
    char *applet_log_file_old = g_build_filename (logdir, "applet-old.log", NULL);

    GStatBuf log_file_stat_buf;
    if (g_stat(applet_log_file, &log_file_stat_buf) != 0) {
        // Do not warn if errno=2 (file not exist), because during
        // first run of the client the log files may not be created
        // yet.
        if (errno != 2) {
            g_warning ("Get log file %s stat failed errno=%d.", applet_log_file, errno);
        }
        goto out;
    }

    const int delete_threshold = 300 * 1000 * 1000;
    if (log_file_stat_buf.st_size <= delete_threshold) {
        goto out;
    }

    seafile_log_reopen (applet_log_file, applet_log_file_old);

out:
    g_free (logdir);
    g_free (applet_log_file);
    g_free (applet_log_file_old);
}
