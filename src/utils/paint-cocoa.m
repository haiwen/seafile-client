#include <Cocoa/Cocoa.h>
#include "paint-cocoa.h"

static bool checked = false;
static float scaleFactor = 1.0;
inline static void checkFactor()
{
    if (!checked){
        if ([[NSScreen mainScreen] respondsToSelector: @selector(backingScaleFactor)])
            scaleFactor = [[NSScreen mainScreen] backingScaleFactor];
        checked = true;
    }
}

int _isHiDPI()
{
    checkFactor();
    return (scaleFactor > 1.0);
}

float _getScaleFactor()
{
    checkFactor();
    return scaleFactor;
}
