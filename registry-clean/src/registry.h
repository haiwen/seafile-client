#ifndef SEAFILE_GUI
#define SEAFILE_GUI
#if defined(Q_OS_WIN32)
#include<windows.h>
#endif

#if defined(Q_OS_WIN32)
bool cleanRegistryItem(HKEY root, const QString path);
#endif

#endif //SEAFILE_GUI
