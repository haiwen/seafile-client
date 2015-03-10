#include "utils-mac.h"

#include <AvailabilityMacros.h>
#import <Cocoa/Cocoa.h>

#include <QString>

#if !__has_feature(objc_arc)
#error this file must be built with ARC support
#endif

// borrowed from AvailabilityMacros.h
#ifndef MAC_OS_X_VERSION_10_10
#define MAC_OS_X_VERSION_10_10      101000
#endif

@interface DarkmodeHelper : NSObject
- (void)getDarkMode;
@end

static bool darkMode = false;
static DarkModeChangedCallback *darkModeWatcher = NULL;
@implementation DarkmodeHelper
- (id) init {
    self = [super init];

    unsigned major;
    unsigned minor;
    unsigned patch;
    utils::mac::get_current_osx_version(&major, &minor, &patch);
    // darkmode is available version >= 10.10
    if (major == 10 && minor >= 10) {
        [self getDarkMode];

        [[NSDistributedNotificationCenter defaultCenter] addObserver:self
        selector:@selector(darkModeChanged:)
        name:@"AppleInterfaceThemeChangedNotification" object:nil];
    }
    return self;
}

- (void)darkModeChanged:(NSNotification *)aNotification
{
    bool oldDarkMode = darkMode;
    [self getDarkMode];

    if (oldDarkMode != darkMode && darkModeWatcher) {
        darkModeWatcher(darkMode);
    }
}

- (void)getDarkMode {
    NSDictionary *dict = [[NSUserDefaults standardUserDefaults] persistentDomainForName:NSGlobalDomain];
    id style = [dict objectForKey:@"AppleInterfaceStyle"];
    darkMode = ( style && [style isKindOfClass:[NSString class]] && NSOrderedSame == [style caseInsensitiveCompare:@"dark"]);
}
@end


namespace utils {
namespace mac {

//TransformProcessType is not encouraged to use, aha
//Sorry but not functional for OSX 10.7
void setDockIconStyle(bool hidden) {
    //https://developer.apple.com/library/mac/documentation/AppKit/Reference/NSRunningApplication_Class/Reference/Reference.html
    if (hidden) {
        [[NSApplication sharedApplication] setActivationPolicy: NSApplicationActivationPolicyAccessory];
    } else {
        [[NSApplication sharedApplication] setActivationPolicy: NSApplicationActivationPolicyRegular];
    }
}

// https://bugreports.qt-project.org/browse/QTBUG-40449 is fixed in QT 5.4.1
// TODO remove this and related code once qt 5.4.1 is widely used
QString fix_file_id_url(const QString &path) {
    if (!path.startsWith("/.file/id="))
        return path;
    const QString url = "file://" + path;
    NSString *fileIdURL = [NSString stringWithCString:url.toUtf8().data()
                                    encoding:NSUTF8StringEncoding];
    NSURL *goodURL = [[NSURL URLWithString:fileIdURL] filePathURL];
    NSString *filePath = goodURL.path; // readonly

    QString retval = QString::fromUtf8([filePath UTF8String],
                                       [filePath lengthOfBytesUsingEncoding:NSUTF8StringEncoding]);
    return retval;
}

// original idea come from growl framework
// http://growl.info/about
bool get_auto_start()
{
    NSURL *itemURL = [[NSBundle mainBundle] bundleURL];
    Boolean found = false;
    CFURLRef URLToToggle = (CFURLRef)CFBridgingRetain(itemURL);
    LSSharedFileListRef loginItems = LSSharedFileListCreate(NULL, kLSSharedFileListSessionLoginItems, NULL);
    if (loginItems) {
        UInt32 seed = 0U;
        NSArray *currentLoginItems = CFBridgingRelease((LSSharedFileListCopySnapshot(loginItems, &seed)));
        for (id itemObject in currentLoginItems) {
            LSSharedFileListItemRef item = (LSSharedFileListItemRef)CFBridgingRetain(itemObject);

            UInt32 resolutionFlags = kLSSharedFileListNoUserInteraction | kLSSharedFileListDoNotMountVolumes;
            CFURLRef URL = NULL;
#if (__MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_10)
            CFErrorRef err;
            URL = LSSharedFileListItemCopyResolvedURL(item, resolutionFlags, &err);
            if (err) {
#else
            OSStatus err = LSSharedFileListItemResolve(item, resolutionFlags, &URL, /*outRef*/ NULL);
            if (err == noErr) {
#endif
                found = CFEqual(URL, URLToToggle);
                CFRelease(URL);

                if (found)
                    break;
            }
        }
        CFRelease(loginItems);
    }
    return found;
}

void set_auto_start(bool enabled)
{
    NSURL *itemURL = [[NSBundle mainBundle] bundleURL];
    OSStatus status;
    CFURLRef URLToToggle = (CFURLRef)CFBridgingRetain(itemURL);
    LSSharedFileListItemRef existingItem = NULL;
    LSSharedFileListRef loginItems = LSSharedFileListCreate(kCFAllocatorDefault, kLSSharedFileListSessionLoginItems, /*options*/ NULL);
    if(loginItems)
    {
        UInt32 seed = 0U;
        NSArray *currentLoginItems = CFBridgingRelease(LSSharedFileListCopySnapshot(loginItems, &seed));
        for (id itemObject in currentLoginItems) {
            LSSharedFileListItemRef item = (LSSharedFileListItemRef)CFBridgingRetain(itemObject);

            UInt32 resolutionFlags = kLSSharedFileListNoUserInteraction | kLSSharedFileListDoNotMountVolumes;
            CFURLRef URL = NULL;
#if (__MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_10)
            CFErrorRef err;
            URL = LSSharedFileListItemCopyResolvedURL(item, resolutionFlags, &err);
            if (err) {
#else
            OSStatus err = LSSharedFileListItemResolve(item, resolutionFlags, &URL, /*outRef*/ NULL);
            if (err == noErr) {
#endif
                Boolean found = CFEqual(URL, URLToToggle);
                CFRelease(URL);

                if (found) {
                    existingItem = item;
                    break;
                }
            }
        }

        if (enabled && (existingItem == NULL)) {
            NSString *displayName = @"Seafile Client";
            IconRef icon = NULL;
            FSRef ref;
            //TODO: replace the deprecated CFURLGetFSRef
            Boolean gotRef = CFURLGetFSRef(URLToToggle, &ref);
            if (gotRef) {
                status = GetIconRefFromFileInfo(&ref,
                                                /*fileNameLength*/ 0,
                                                /*fileName*/ NULL,
                                                kFSCatInfoNone,
                                                /*catalogInfo*/ NULL,
                                                kIconServicesNormalUsageFlag,
                                                &icon,
                                                /*outLabel*/ NULL);
                if (status != noErr)
                    icon = NULL;
            }

            LSSharedFileListInsertItemURL(loginItems, kLSSharedFileListItemBeforeFirst,
                  (CFStringRef)CFBridgingRetain(displayName), icon, URLToToggle,
                  /*propertiesToSet*/ NULL, /*propertiesToClear*/ NULL);
          } else if (!enabled && (existingItem != NULL))
              LSSharedFileListItemRemove(loginItems, existingItem);

        CFRelease(loginItems);
    }
}

bool is_darkmode() {
    static DarkmodeHelper *helper = nil;
    if (!helper) {
        helper = [[DarkmodeHelper alloc] init];
    }
    return darkMode;
}
void set_darkmode_watcher(DarkModeChangedCallback *cb) {
    darkModeWatcher = cb;
}

void get_current_osx_version(unsigned *major, unsigned *minor, unsigned *patch) {
#if (__MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_10)
    NSOperatingSystemVersion version = [[NSProcessInfo processInfo] operatingSystemVersion];
    *major = version.majorVersion;
    *minor = version.minorVersion;
    *patch = version.patchVersion;
#else
    NSString *versionString = [[NSProcessInfo processInfo] operatingSystemVersionString];
    NSArray *array = [versionString componentsSeparatedByString:@" "];
    if (array.count < 2) {
        *major = 10;
        *minor = 7;
        *patch = 0;
        return;
    }
    NSArray *versionArray = [array[1] componentsSeparatedByString:@"."];
    if (versionArray.count < 3) {
        *major = 10;
        *minor = 7;
        *patch = 0;
        return;
    }
    *major = [versionArray[0] intValue];
    *minor = [versionArray[1] intValue];
    *patch = [versionArray[2] intValue];
#endif
}

} // namespace mac
} // namespace utils
