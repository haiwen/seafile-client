#include "process.h"

#include <sys/sysctl.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <glib.h>

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
    pid_t current_pid = getpid();
    GetBSDProcessList (&mylist, &mycount);
    for (size_t k = 0; k < mycount; k++) {
        kinfo_proc *proc =  &mylist[k];
        if (strcmp (proc->kp_proc.p_comm, name) == 0 &&
            proc->kp_proc.p_pid != current_pid){
            kill (proc->kp_proc.p_pid, SIGKILL);
        }
    }
    free (mylist);
}

int count_process(const char *process_name)
{
    int count = 0;
    struct kinfo_proc *mylist = NULL;
    size_t mycount = 0;
    GetBSDProcessList (&mylist, &mycount);
    for (size_t k = 0; k < mycount; k++) {
        kinfo_proc *proc =  &mylist[k];
        if (strcmp (proc->kp_proc.p_comm, process_name) == 0){
            count++;
        }
    }
    free (mylist);
    return count;
}
