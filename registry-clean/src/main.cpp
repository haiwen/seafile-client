#include <QString>

#include "registry.h"

namespace {
	const QString kRGISTRYPATH = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ShellIconOverlayIdentifiers";
} //namespace

int main()
{
#if defined(Q_OS_WIN32)
	cleanRegistryItem(HKEY_LOCAL_MACHINE, kRGISTRYPATH);
#endif
}
