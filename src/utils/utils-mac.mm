#include <AvailabilityMacros.h>
#include <Cocoa/Cocoa.h>
#include "utils-mac.h"
#include <QString>

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
        [[NSApplication sharedApplication] setActivationPolicy: NSApplicationActivationPolicyAccessory];
    } else {
        [[NSApplication sharedApplication] setActivationPolicy: NSApplicationActivationPolicyRegular];
    }
}

// Yosemite uses a new url format called fileId url, use this helper to transform
// it to the old style.
// https://bugreports.qt-project.org/browse/QTBUG-40449
// NSString *fileIdURL = @"file:///.file/id=6571367.1000358";
// NSString *goodURL = [[NSURL URLWithString:fileIdURL] filePathURL];
QString __mac_get_path_from_fileId_url(const QString &url) {
    NSString *fileIdURL = [NSString stringWithCString:url.toUtf8().data()
                                    encoding:NSUTF8StringEncoding];
    NSURL *goodURL = [[NSURL URLWithString:fileIdURL] filePathURL];
    NSString *filePath = goodURL.path; // readonly

    QString retval = QString::fromUtf8([filePath UTF8String],
                                       [filePath lengthOfBytesUsingEncoding:NSUTF8StringEncoding]);
#if !__has_feature(objc_arc)
#   error this file must be built with ARC support
#endif
    return retval;
}

