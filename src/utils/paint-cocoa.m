#include <AvailabilityMacros.h>
#include <Cocoa/Cocoa.h>
#include "paint-cocoa.h"

static bool checked = false;
static double scaleFactor = 1.0;
inline static void checkFactor()
{
    if (!checked){
#if (MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_7)
        if ([[NSScreen mainScreen] respondsToSelector: @selector(backingScaleFactor)])
            scaleFactor = [[NSScreen mainScreen] backingScaleFactor];
#else
        scaleFactor = 1.0;
#endif
        checked = true;
    }
}

int _isHiDPI()
{
    checkFactor();
    return (scaleFactor > 1.0);
}

double _getScaleFactor()
{
    checkFactor();
    return scaleFactor;
}
