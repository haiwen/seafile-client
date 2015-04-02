#ifndef SEAFILE_CLIENT_EXTENSION_LOG_H
#define SEAFILE_CLIENT_EXTENSION_LOG_H

void seaf_ext_log_start();
void seaf_ext_log_stop();

void seaf_ext_log_aux(const char *format, ...);

#define seaf_ext_log(format, ... )                                  \
    seaf_ext_log_aux("%s(line %d) %s: " format,                     \
                     __FILE__, __LINE__, __func__, ##__VA_ARGS__)   \


#endif  // SEAFILE_CLIENT_EXTENSION_LOG_H
