#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <glib.h>

#include "process.h"
#if !defined(PATH_MAX)
#define PATH_MAX 512
#endif
namespace {
const int kBUFFSIZE = 4096;
}

static int
find_process_in_dirent(struct dirent *dir, const char *process_name)
{
    char path[PATH_MAX];
    /* first construct a path like /proc/123/exe */
    if (snprintf (path, PATH_MAX, "/proc/%s/exe", dir->d_name) < 0) {
        return -1;
    }

    char buf[kBUFFSIZE];
    /* get the full path of exe */
    ssize_t l = readlink(path, buf, kBUFFSIZE - 1);

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
    pid_t current_pid = getpid();
    while ((subdir = readdir(proc_dir))) {
        char first = subdir->d_name[0];
        /* /proc/[1-9][0-9]* */
        if (first > '9' || first < '1')
            continue;
        int pid = find_process_in_dirent(subdir, name);
        // don't kill itself!
        if (pid > 0 && pid != current_pid) {
            kill (pid, SIGKILL);
        }
    }

    closedir(proc_dir);
}

int count_process(const char *process_name)
{
    int count = 0;
    DIR *proc_dir = opendir("/proc");
    if (!proc_dir) {
        g_warning ("failed to open /proc/ :%s\n", strerror(errno));
        return FALSE;
    }

    struct dirent *subdir = NULL;
    while ((subdir = readdir(proc_dir))) {
        char first = subdir->d_name[0];
        /* /proc/[1-9][0-9]* */
        if (first > '9' || first < '1')
            continue;
        if (find_process_in_dirent(subdir, process_name) > 0) {
            count++;
        }
    }

    closedir (proc_dir);
    return count;
}

