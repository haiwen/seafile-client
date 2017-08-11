#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif
#include <windows.h>
#include <psapi.h>

#include <assert.h>
#include <errno.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

#include <QList>

#include "process.h"

namespace {

/// always ignore current process
HANDLE
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
    DWORD hCurrentProcessId = GetCurrentProcessId();
    unsigned int i;
    DWORD length;

    for (i = 0; i < cProcesses; i++) {
        if(aProcesses[i] == 0 || aProcesses[i] == hCurrentProcessId)
            continue;
        hProcess = OpenProcess (PROCESS_QUERY_INFORMATION |
                                PROCESS_TERMINATE |
                                PROCESS_VM_READ |
                                SYNCHRONIZE , FALSE, aProcesses[i]);
        if (!hProcess)
            continue;

        char process_name[4096] = {0};

        // The GetProcessImageFileName function returns the path in device form, rather than drive letters
        // https://msdn.microsoft.com/en-us/library/windows/desktop/ms683196%28v=vs.85%29.aspx
        // https://msdn.microsoft.com/en-us/library/windows/desktop/ms683217%28v=vs.85%29.aspx
        if (!(length = GetProcessImageFileName(hProcess, process_name, sizeof(process_name)))) {
            CloseHandle(hProcess);
            continue;
        }
        char *basename = strrchr(process_name, '\\');
        length -= (basename - process_name);

        // if basename doesn't start with `\` or not mached
        if (*basename != '\\' ||
            strncasecmp(name, ++basename, length) != 0) {
            CloseHandle(hProcess);
            continue;
        }

        return hProcess;
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


} // namespace

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

void shutdown_process (const char *name)
{
    while (win32_kill_process(name) >= 0) ;
}

// Return the number of processes with the given executable name, and also
// return the list of pids of them (except the current process, if it mathces)
static int count_process_internal (const char *process_name_in, QList<uint64_t> *pids)
{
    char name[MAX_PATH];
    DWORD aProcesses[1024], cbNeeded, cProcesses;
    HANDLE hProcess;
    DWORD length;
    int count = 0;
    DWORD i;
    DWORD hCurrentProcessId = GetCurrentProcessId();

    if (strstr(process_name_in, ".exe")) {
        snprintf (name, sizeof(name), "%s", process_name_in);
    } else {
        snprintf (name, sizeof(name), "%s.exe", process_name_in);
    }

    if (!EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded)) {
        return 0;
    }

    /* Calculate how many process identifiers were returned. */
    cProcesses = cbNeeded / sizeof(DWORD);

    for (i = 0; i < cProcesses; i++) {
        if(aProcesses[i] == 0)
            continue;
        hProcess = OpenProcess (PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, aProcesses[i]);
        if (!hProcess) {
            continue;
        }

        char process_name[4096] = {0};
        if (!(length = GetProcessImageFileName(hProcess, process_name, sizeof(process_name)))) {
            CloseHandle(hProcess);
            continue;
        }
        char *basename = strrchr(process_name, '\\');
        length -= (basename - process_name);

        // if basename doesn't start with `\` or not mached
        if (*basename != '\\' ||
            strncasecmp(name, ++basename, length) != 0) {
            CloseHandle(hProcess);
            continue;
        }

        if (pids && aProcesses[i] != hCurrentProcessId) {
            pids->append(aProcesses[i]);
        }
        count++;
        CloseHandle(hProcess);
    }

    return count;
}

int count_process (const char *process_name_in) {
    return count_process_internal(process_name_in, nullptr);
}

int count_process(const char *name, uint64_t *pid)
{
    QList<uint64_t> pids;
    int ret = count_process_internal(name, &pids);
    if (pid && !pids.isEmpty()) {
        *pid = pids[0];
    }
    return ret;
}
