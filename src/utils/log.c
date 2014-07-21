#include <errno.h>
#include <time.h>
#include <glib/gstdio.h>

#include "log.h"

static FILE *logfp;

static GLogLevelFlags applet_log_level;

static int
checkdir_with_mkdir (const char *dir)
{
#if defined(Q_OS_WIN)
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
    if (log_level > applet_log_level)
        return;

    if (log_level & G_LOG_FLAG_FATAL)
        fputs (message, stderr);

    time_t t;
    struct tm *tm;
    char buf[1024];
    size_t len;

    if (log_level > applet_log_level)
        return;

    t = time(NULL);
    tm = localtime(&t);
    len = strftime (buf, 1024, "[%x %X] ", tm);
    g_return_if_fail (len < 1024);
    fputs (buf, logfp);
    fputs (message, logfp);

    if (message && strlen(message) > 0 && message[strlen(message) - 1] != '\n') {
        fputs("\n", logfp);
    }
    fflush (logfp);
}

int
applet_log_init (const char *ccnet_dir)
{
    char *logdir = g_build_filename (ccnet_dir, "logs", NULL);
    char *file = g_build_filename(logdir, "applet.log", NULL);

    checkdir_with_mkdir (logdir);
    g_free (logdir);

    g_log_set_handler (NULL, G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL
                       | G_LOG_FLAG_RECURSION, applet_log, NULL);

    g_log_set_handler ("Ccnet", G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL
                       | G_LOG_FLAG_RECURSION, applet_log, NULL);

    /* record all log message */
    applet_log_level = G_LOG_LEVEL_DEBUG;

    if ((logfp = (FILE *)(long)g_fopen (file, "a+")) == NULL) {
        g_warning ("Open file %s failed errno=%d\n", file, errno);
        g_free (file);
        return -1;
    }

    g_free (file);

    return 0;
}

