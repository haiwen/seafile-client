#include <glib-object.h>
#include <QTimer>
#include <QStringList>
#include <QString>
#include <QDebug>

extern "C" {
#include <ccnet/ccnet-client.h>
}


#include "daemon-mgr.h"
#include "configurator.h"
#include "seafile-applet.h"

namespace {

const int kCheckIntervalMilli = 5000;
const int kConnDaemonIntervalMilli = 1000;

#if defined(Q_WS_WIN)
const char *kCcnetDaemonExecutable = "ccnet.exe";
const char *kSeafileDaemonExecutable = "seaf-daemon.exe";
#else
const char *kCcnetDaemonExecutable = "ccnet";
const char *kSeafileDaemonExecutable = "seaf-daemon";
#endif

#if defined(Q_WS_MAC)
#include <sys/sysctl.h>
#include <assert.h>
#include <errno.h>

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
    for (int k = 0; k < mycount; k++) {
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

void shutdown_process (const char *name)
{
    struct kinfo_proc *mylist = NULL;
    size_t mycount = 0;
    GetBSDProcessList (&mylist, &mycount);
    for (int k = 0; k < mycount; k++) {
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
    char process_name[SEAF_PATH_MAX];
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

    char buf[SEAF_PATH_MAX];
    /* get the full path of exe */
    ssize_t l = readlink(path, buf, SEAF_PATH_MAX);

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

} // namespace




DaemonManager::DaemonManager()
    : sync_client_(0),
      ccnet_daemon_(0),
      seaf_daemon_(0)
{
    monitor_timer_ = new QTimer(this);
    conn_daemon_timer_ = new QTimer(this);
    connect(conn_daemon_timer_, SIGNAL(timeout()), this, SLOT(tryConnCcnet()));
    shutdown_process (kCcnetDaemonExecutable);
}

void DaemonManager::startCcnetDaemon()
{
    sync_client_ = ccnet_client_new();

    const QString config_dir = seafApplet->configurator()->ccnetDir();
    const QByteArray path = config_dir.toUtf8();
    if (ccnet_client_load_confdir(sync_client_, path.data()) <  0) {
        seafApplet->errorAndExit(tr("failed to load ccnet config dir %1").arg(config_dir));
    }

    ccnet_daemon_ = new QProcess(this);
    connect(ccnet_daemon_, SIGNAL(started()), this, SLOT(onCcnetDaemonStarted()));
    connect(ccnet_daemon_, SIGNAL(finished(int, QProcess::ExitStatus)),
            this, SLOT(onCcnetDaemonExited()));

    QStringList args;
    args << "-c" << config_dir << "-D" << "Peer,Message,Connection,Other";
    ccnet_daemon_->start(kCcnetDaemonExecutable, args);
}

void DaemonManager::startSeafileDaemon()
{
    const QString config_dir = seafApplet->configurator()->ccnetDir();
    const QString seafile_dir = seafApplet->configurator()->seafileDir();
    const QString worktree_dir = seafApplet->configurator()->worktreeDir();

    seaf_daemon_ = new QProcess(this);
    connect(seaf_daemon_, SIGNAL(started()), this, SLOT(onSeafDaemonStarted()));
    connect(ccnet_daemon_, SIGNAL(finished(int, QProcess::ExitStatus)),
            this, SLOT(onSeafDaemonExited()));

    QStringList args;
    args << "-c" << config_dir << "-d" << seafile_dir << "-w" << worktree_dir;
    seaf_daemon_->start(kSeafileDaemonExecutable, args);

    qDebug() << "starting seaf-daemon: " << args;
}

void DaemonManager::onCcnetDaemonStarted()
{
    conn_daemon_timer_->start(kConnDaemonIntervalMilli);
}

void DaemonManager::onSeafDaemonStarted()
{
    qDebug("seafile daemon is now running");
    emit daemonStarted();
}

void DaemonManager::onCcnetDaemonExited()
{
    if (seafApplet->inExit()) {
        return;
    }

    seafApplet->errorAndExit(tr("ccnet daemon has exited abnormally"));
}

void DaemonManager::onSeafDaemonExited()
{
    if (seafApplet->inExit()) {
        return;
    }

    seafApplet->errorAndExit(tr("seafile daemon has exited abnormally"));
}

void DaemonManager::stopAll()
{
    qDebug("[Daemon Mgr] stopping ccnet/seafile daemon");
    if (seaf_daemon_)
        seaf_daemon_->terminate();
    if (ccnet_daemon_)
        ccnet_daemon_->terminate();
}

void DaemonManager::tryConnCcnet()
{
    qDebug("trying to connect to ccnet daemon...\n");

    if (ccnet_client_connect_daemon(sync_client_, CCNET_CLIENT_SYNC) < 0) {
        return;
    } else {
        conn_daemon_timer_->stop();

        qDebug("connected to ccnet daemon\n");

        startSeafileDaemon();
    }
}
