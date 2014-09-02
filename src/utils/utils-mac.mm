#include <AvailabilityMacros.h>
#include <Cocoa/Cocoa.h>
#include "utils-mac.h"

static bool checked = false;
static double scaleFactor = 1.0;
inline static void checkFactor() {
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

int __mac_isHiDPI() {
    checkFactor();
    return (scaleFactor > 1.0);
}

double __mac_getScaleFactor() {
    checkFactor();
    return scaleFactor;
}

//TransformProcessType is not encouraged to use, aha
//Sorry but not functional for OSX 10.7
void __mac_setDockIconStyle(bool hidden) {
    //https://developer.apple.com/library/mac/documentation/AppKit/Reference/NSRunningApplication_Class/Reference/Reference.html
    if (hidden) {
        [NSApp setActivationPolicy: NSApplicationActivationPolicyAccessory];
    } else {
        [NSApp setActivationPolicy: NSApplicationActivationPolicyRegular];
    }
}
