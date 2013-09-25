#ifndef SEAFILE_CLIENT_UTILS_LOG_H
#define SEAFILE_CLIENT_UTILS_LOG_H

#include <glib.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

G_BEGIN_DECLS

#ifdef __APPLE__
#define __BASEFILE__ ((strrchr(__FILE__, '/') ?: __FILE__ - 1) + 1)
#else
#define __BASEFILE__ __FILE__
#endif

int applet_log_init (const char *ccnet_dir);

G_END_DECLS

#endif // SEAFILE_CLIENT_UTILS_LOG_H
