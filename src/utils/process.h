#ifndef SEAFILE_CLIENT_UTILS_PROCESS_H
#define SEAFILE_CLIENT_UTILS_PROCESS_H

#include <stdint.h>

// process related functions
int process_is_running (const char *process_name);

void shutdown_process (const char *name);

// Return the number of processes whose executable name is `name`.
int count_process(const char *name);

// Same as count_process(name), but also return the pid of the first matched
// non-self process.
int count_process(const char *name, uint64_t *pid);

#endif // SEAFILE_CLIENT_UTILS_PROCESS_H
