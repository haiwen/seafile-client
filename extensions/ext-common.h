#ifndef SEAFILE_EXT_EXT_COMMON_H
#define SEAFILE_EXT_EXT_COMMON_H

// This header should be the first include of each cpp source file

// A workaround for some mingw problem.
// See http://stackoverflow.com/questions/3445312/swprintf-and-vswprintf-not-declared
#undef __STRICT_ANSI__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>

#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <commctrl.h>

#include <shlobj.h>
#include <shlguid.h>
#include <wininet.h>
#include <aclapi.h>

#endif // SEAFILE_EXT_EXT_COMMON_H
