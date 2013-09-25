
#include <assert.h>
#include <errno.h>
#include <dirent.h>
#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sqlite3.h>
#include <glib.h>
#include <string.h>
#include <QString>
#include <string>
#include <jansson.h>

#if defined(Q_WS_MAC)
    #include <sys/sysctl.h>
#elif defined(Q_WS_WIN)
    #include <windows.h>
    #include <psapi.h>
#endif

#include <QMap>
#include <QVariant>
#include <QDebug>
#include <QDateTime>

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

#if defined(Q_WS_MAC)

typedef struct kinfo_proc kinfo_proc;

static int GetBSDProcessList (kinfo_proc **procList, size_t *procCount)
{
    int                 err;
    kinfo_proc *        result;
    bool                done;
    static const int    name[] = { CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0 };
    // Declaring name as const requires us to cast it when passing it to
    // sysctl because the prototype doesn't include the const modifier.
    size_t              length;

    assert ( procList != NULL);
    assert (*procList == NULL);
    assert (procCount != NULL);

    *procCount = 0;

    // We start by calling sysctl with result == NULL and length == 0.
    // That will succeed, and set length to the appropriate length.
    // We then allocate a buffer of that size and call sysctl again
    // with that buffer.  If that succeeds, we're done.  If that fails
    // with ENOMEM, we have to throw away our buffer and loop.  Note
    // that the loop causes use to call sysctl with NULL again; this
    // is necessary because the ENOMEM failure case sets length to
    // the amount of data returned, not the amount of data that
    // could have been returned.

    result = NULL;
    done = false;
    do {
        assert (result == NULL);
        // Call sysctl with a NULL buffer.

        length = 0;
        err = sysctl ((int *) name, (sizeof(name) / sizeof(*name)) - 1,
                     NULL, &length,
                     NULL, 0);
        if (err == -1) {
            err = errno;
        }

        // Allocate an appropriately sized buffer based on the results
        // from the previous call.

        if (err == 0) {
            result = (kinfo_proc *)malloc (length);
            if (result == NULL) {
                err = ENOMEM;
            }
        }

        // Call sysctl again with the new buffer.  If we get an ENOMEM
        // error, toss away our buffer and start again.

        if (err == 0) {
            err = sysctl ((int *) name, (sizeof(name) / sizeof(*name)) - 1,
                         result, &length,
                         NULL, 0);
            if (err == -1) {
                err = errno;
            }
            if (err == 0) {
                done = true;
            } else if (err == ENOMEM) {
                assert(result != NULL);
                free (result);
                result = NULL;
                err = 0;
            }
        }
    } while (err == 0 && ! done);

    // Clean up and establish post conditions.

    if (err != 0 && result != NULL) {
        free (result);
        result = NULL;
    }
    *procList = result;
    if (err == 0) {
        *procCount = length / sizeof(kinfo_proc);
    }

    assert ( (err == 0) == (*procList != NULL) );

    return err;
}

static int getBSDProcessPid (const char *name, int except_pid)
{
    int pid = 0;
    struct kinfo_proc *mylist = NULL;
    size_t mycount = 0;
    GetBSDProcessList (&mylist, &mycount);
    for (size_t k = 0; k < mycount; k++) {
        kinfo_proc *proc =  &mylist[k];
        if (proc->kp_proc.p_pid != except_pid
            && strcmp (proc->kp_proc.p_comm, name) == 0){
            pid = proc->kp_proc.p_pid;
            break;
        }
    }
    free (mylist);
    return pid;
}

int process_is_running (const char *name)
{
    int pid = getBSDProcessPid (name, getpid ());
    if (pid)
        return true;
    return false;
}


void shutdown_process (const char *name)
{
    struct kinfo_proc *mylist = NULL;
    size_t mycount = 0;
    GetBSDProcessList (&mylist, &mycount);
    for (size_t k = 0; k < mycount; k++) {
        kinfo_proc *proc =  &mylist[k];
        if (strcmp (proc->kp_proc.p_comm, name) == 0){
            kill (proc->kp_proc.p_pid, SIGKILL);
        }
    }
    free (mylist);
}

#elif defined(Q_WS_WIN)
static HANDLE
get_process_handle (const char *process_name_in)
{
    char name[256];
    if (strstr(process_name_in, ".exe")) {
        snprintf (name, sizeof(name), "%s", process_name_in);
    } else {
        snprintf (name, sizeof(name), "%s.exe", process_name_in);
    }

    DWORD aProcesses[1024], cbNeeded, cProcesses;

    if (!EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded))
        return NULL;

    /* Calculate how many process identifiers were returned. */
    cProcesses = cbNeeded / sizeof(DWORD);

    HANDLE hProcess;
    HMODULE hMod;
    char process_name[4096];
    unsigned int i;

    for (i = 0; i < cProcesses; i++) {
        if(aProcesses[i] == 0)
            continue;
        hProcess = OpenProcess (PROCESS_ALL_ACCESS, FALSE, aProcesses[i]);
        if (!hProcess)
            continue;

        if (EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded)) {
            GetModuleBaseName(hProcess, hMod, process_name,
                              sizeof(process_name)/sizeof(char));
        }

        if (strcasecmp(process_name, name) == 0)
            return hProcess;
        else {
            CloseHandle(hProcess);
        }
    }
    /* Not found */
    return NULL;
}

int
process_is_running (const char *process_name)
{
    HANDLE proc_handle = get_process_handle(process_name);

    if (proc_handle) {
        CloseHandle(proc_handle);
        return TRUE;
    } else {
        return FALSE;
    }
}

int
win32_kill_process (const char *process_name)
{
    HANDLE proc_handle = get_process_handle(process_name);

    if (proc_handle) {
        TerminateProcess(proc_handle, 0);
        CloseHandle(proc_handle);
        return 0;
    } else {
        return -1;
    }
}

void shutdown_process (const char *name)
{
    while (win32_kill_process(name) >= 0) ;
}

#else
static int
find_process_in_dirent(struct dirent *dir, const char *process_name)
{
    char path[512];
    /* fisrst construct a path like /proc/123/exe */
    if (sprintf (path, "/proc/%s/exe", dir->d_name) < 0) {
        return -1;
    }

    char buf[4096];
    /* get the full path of exe */
    ssize_t l = readlink(path, buf, 4096);

    if (l < 0)
        return -1;
    buf[l] = '\0';

    /* get the base name of exe */
    char *base = g_path_get_basename(buf);
    int ret = strcmp(base, process_name);
    g_free(base);

    if (ret == 0)
        return atoi(dir->d_name);
    else
        return -1;
}

/* read the /proc fs to determine whether some process is running */
int process_is_running (const char *process_name)
{
    DIR *proc_dir = opendir("/proc");
    if (!proc_dir) {
        fprintf (stderr, "failed to open /proc/ dir\n");
        return FALSE;
    }

    struct dirent *subdir = NULL;
    while ((subdir = readdir(proc_dir))) {
        char first = subdir->d_name[0];
        /* /proc/[1-9][0-9]* */
        if (first > '9' || first < '1')
            continue;
        int pid = find_process_in_dirent(subdir, process_name);
        if (pid > 0) {
            closedir(proc_dir);
            return TRUE;
        }
    }

    closedir(proc_dir);
    return FALSE;
}

void shutdown_process (const char *name)
{
    DIR *proc_dir = opendir("/proc");
    if (!proc_dir) {
        fprintf (stderr, "failed to open /proc/ dir\n");
        return;
    }

    struct dirent *subdir = NULL;
    while ((subdir = readdir(proc_dir))) {
        char first = subdir->d_name[0];
        /* /proc/[1-9][0-9]* */
        if (first > '9' || first < '1')
            continue;
        int pid = find_process_in_dirent(subdir, name);
        if (pid > 0) {
            kill (pid, SIGKILL);
        }
    }

    closedir(proc_dir);
}

#endif

#if defined(Q_WS_WIN)
static LONG
get_win_run_key (HKEY *pKey)
{
    const char *key_run = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run";
    LONG result = RegOpenKeyEx(
        /* We don't use HKEY_LOCAL_MACHINE here because that requires
         * seaf-daemon to run with admin privilege. */
                               HKEY_CURRENT_USER,
                               key_run,
                               0L,KEY_WRITE | KEY_READ,
                               pKey);
    if (result != ERROR_SUCCESS) {
        qDebug("Failed to open Registry key %s\n", key_run);
    }

    return result;
}

static int
add_to_auto_start (const wchar_t *appname_w, const wchar_t *path_w)
{
    HKEY hKey;
    LONG result = get_win_run_key(&hKey);
    if (result != ERROR_SUCCESS) {
        return -1;
    }

    DWORD n = sizeof(wchar_t) * (wcslen(path_w) + 1);

    result = RegSetValueExW (hKey, appname_w,
                             0, REG_SZ, (const BYTE *)path_w, n);

    RegCloseKey(hKey);
    if (result != ERROR_SUCCESS) {
        qDebug("Failed to create auto start value\n");
        return -1;
    }

    return 0;
}

static int
delete_from_auto_start(const char *appname)
{
    HKEY hKey;
    LONG result = get_win_run_key(&hKey);
    if (result != ERROR_SUCCESS) {
        return -1;
    }

    result = RegDeleteValue (hKey, appname);
    RegCloseKey(hKey);
    if (result != ERROR_SUCCESS) {
        qDebug("Failed to remove auto start value for %s\n", appname);
        return -1;
    }

    return 0;
}

int
get_seafile_auto_start()
{
    HKEY hKey;
    LONG result = get_win_run_key(&hKey);
    if (result != ERROR_SUCCESS) {
        return -1;
    }

    char buf[MAX_PATH] = {0};
    DWORD len = sizeof(buf);
    result = RegQueryValueEx (hKey,             /* Key */
                              "Seafile",        /* value */
                              NULL,             /* reserved */
                              NULL,             /* output type */
                              (LPBYTE)buf,      /* output data */
                              &len);            /* output length */

    RegCloseKey(hKey);
    if (result != ERROR_SUCCESS) {
        /* seafile applet auto start no set  */
        return 0;
    }

    return 1;
}

int
set_seafile_auto_start(int on)
{
    int result = 0;
    if (on) {
        /* turn on auto start  */
        wchar_t applet_path[MAX_PATH];
        if (GetModuleFileNameW (NULL, applet_path, MAX_PATH) == 0) {
            return -1;
        }

        result = add_to_auto_start (L"Seafile", applet_path);

    } else {
        /* turn off auto start */
        result = delete_from_auto_start("Seafile");
    }
    return result;
}

#else
int
get_seafile_auto_start()
{
    return 0;
}

int
set_seafile_auto_start(int /* on */)
{
    return 0;
}

#endif

void open_dir(const QString& path)
{
#ifdef Q_WS_WIN

    std::wstring ws = path.toStdWString();
    ShellExecuteW (0, L"open", ws.c_str(), NULL, NULL, SW_SHOWNORMAL);

#elif defined(Q_WS_MAC)

    // TODO: open_dir in mac
    return;

#else
    char buf[4096];
    snprintf (buf, 4096, "xdg-open '%s' &", toCStr(path));
    if (system (buf) < 0) {
        qDebug("failed to exec: %s", buf);
    }
#endif
}

bool parse_key_value_pairs (char *string, KeyValueFunc func, void *data)
{
    char *line = string, *next, *space;
    char *key, *value;

    while (*line) {
        /* handle empty line */
        if (*line == '\n') {
            ++line;
            continue;
        }

        for (next = line; *next != '\n' && *next; ++next) ;
        *next = '\0';

        for (space = line; space < next && *space != ' '; ++space) ;
        if (*space != ' ') {
            return false;
        }
        *space = '\0';
        key = line;
        value = space + 1;

        if (func(data, key, value) == FALSE)
            return false;

        line = next + 1;
    }
    return true;
}

QMap<QString, QVariant> mapFromJSON(json_t *json, json_error_t */* error */)
{
    QMap<QString, QVariant> dict;
    void *member;
    const char *key;
    json_t *value;

    for (member = json_object_iter(json); member; member = json_object_iter_next(json, member)) {
        key = json_object_iter_key(member);
        value = json_object_iter_value(member);

        QString k = QString::fromUtf8(key);
        QVariant v;

        // json_is_object(const json_t *json)
        // json_is_array(const json_t *json)
        // json_is_string(const json_t *json)
        // json_is_integer(const json_t *json)
        // json_is_real(const json_t *json)
        // json_is_true(const json_t *json)
        // json_is_false(const json_t *json)
        // json_is_null(const json_t *json)
        if (json_is_string(value)) {
            v = json_string_value(value);
        } else if (json_is_integer(value)) {
            v = json_integer_value(value);
        } else if (json_is_real(value)) {
            v = json_real_value(value);
        } else if (json_is_boolean(value)) {
            v = json_is_true(value);
        }

        if (v.isValid()) {
            dict[k] = v;
        }
    }
    return dict;
}

QString translateCommitTime(qint64 timestamp) {
    timestamp *= 1000;          // use milli seconds
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (now <= timestamp) {
        return QObject::tr("Just now");
    }

    qint64 delta = (now - timestamp) / 1000;

    qint64 secondsPerDay = 24 * 60 * 60;

    qint64 days = delta / secondsPerDay;
    qint64 seconds = delta % secondsPerDay;

    QDateTime dt = QDateTime::fromMSecsSinceEpoch(timestamp);

    if (days >= 14) {
        return dt.toString("yyyy-MM-dd");

    } else if (days > 0) {
        return days == 1 ? QObject::tr("1 day ago") : QObject::tr("%1 days ago").arg(days);

    } else if (seconds >= 60 * 60) {
        qint64 hours = seconds / 3600;
        return hours == 1 ? QObject::tr("1 hour ago") : QObject::tr("%1 hours ago").arg(hours);

    } else if (seconds >= 60) {
        qint64 minutes = seconds / 60;
        return minutes == 1 ? QObject::tr("1 minute ago") : QObject::tr("%1 minutes ago").arg(minutes);

    } else if (seconds > 0) {
        return seconds == 1 ? QObject::tr("1 second ago") : QObject::tr("%1 seconds ago").arg(seconds);

    } else {
        return QObject::tr("Just now");
    }
}
