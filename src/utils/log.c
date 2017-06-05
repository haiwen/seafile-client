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

static int
delete_large_file(const char* file)
{
    const int delete_threshold = 300 * 1000 * 1000;

    GStatBuf stat_buf;
    if (g_stat(file, &stat_buf) != 0) {
        g_warning ("Get file %s stat failed errno=%d.", file, errno);
        return -2;
    }

    if (stat_buf.st_size > delete_threshold) {
        if (g_remove(file) == 0) {
            g_warning ("Deleted log file %s.", file);
            return 0;
        } else {
            g_warning ("Delete file %s failed errno=%d.", file, errno);
            return -1;
        }
    } else {
        return 1;
    }
}

int
applet_log_init (const char *ccnet_dir)
{
    char *logdir = g_build_filename (ccnet_dir, "logs", NULL);
    char *applet_file = g_build_filename(logdir, "applet.log", NULL);
    char *seafile_file = g_build_filename(logdir, "seafile.log", NULL);

    if (checkdir_with_mkdir (logdir) < 0) {
        g_free (logdir);
        return -1;
    }

    g_free (logdir);

    delete_large_file(applet_file);
    delete_large_file(seafile_file);

    /* record all log message */
    applet_log_level = G_LOG_LEVEL_DEBUG;

    if ((logfp = g_fopen (applet_file, "a+")) == NULL) {
        g_warning ("Open file %s failed errno=%d\n", applet_file, errno);
        g_free (applet_file);
        return -1;
    }

    g_log_set_handler (NULL, G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL
                       | G_LOG_FLAG_RECURSION, applet_log, NULL);

    g_log_set_handler ("Ccnet", G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL
                       | G_LOG_FLAG_RECURSION, applet_log, NULL);

    g_free (applet_file);

    return 0;
}
