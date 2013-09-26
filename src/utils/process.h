#ifndef SEAFILE_CLIENT_UTILS_PROCESS_H
#define SEAFILE_CLIENT_UTILS_PROCESS_H

// process related functions
int process_is_running (const char *process_name);

void shutdown_process (const char *name);

int count_process(const char *name);

#endif // SEAFILE_CLIENT_UTILS_PROCESS_H
