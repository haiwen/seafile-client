#include "utils-mac.h"

#include <AvailabilityMacros.h>
#import <Cocoa/Cocoa.h>
#import <Security/Security.h>

#include <QString>

#if !__has_feature(objc_arc)
#error this file must be built with ARC support
#endif

// borrowed from AvailabilityMacros.h
#ifndef MAC_OS_X_VERSION_10_10
#define MAC_OS_X_VERSION_10_10      101000
#endif
#ifndef MAC_OS_X_VERSION_10_9
#define MAC_OS_X_VERSION_10_9         1090
#endif
#ifndef MAC_OS_X_VERSION_10_8
#define MAC_OS_X_VERSION_10_8         1080
#endif
#ifndef MAC_OS_X_VERSION_10_7
#define MAC_OS_X_VERSION_10_7         1070
#endif

// ****************************************************************************
// utils::mac::getSystemVersion
// utils::mac::isAtLeastSystemVersion
// ****************************************************************************

namespace utils {
namespace mac {
namespace {
unsigned osver_major = 0;
unsigned osver_minor = 0;
unsigned osver_patch = 0;
inline bool isInitializedSystemVersion() { return osver_major != 0; }
inline void initializeSystemVersion() {
    if (isInitializedSystemVersion())
        return;
#if (__MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_10)
    NSOperatingSystemVersion version = [[NSProcessInfo processInfo] operatingSystemVersion];
    osver_major = version.majorVersion;
    osver_minor = version.minorVersion;
    osver_patch = version.patchVersion;
#else
    NSString *versionString = [[NSProcessInfo processInfo] operatingSystemVersionString];
    NSArray *array = [versionString componentsSeparatedByString:@" "];
    if (array.count < 2) {
        osver_major = 10;
        osver_minor = 7;
        osver_patch = 0;
        return;
    }
    NSArray *versionArray = [[array objectAtIndex:1] componentsSeparatedByString:@"."];
    if (versionArray.count < 3) {
        osver_major = 10;
        osver_minor = 7;
        osver_patch = 0;
        return;
    }
    osver_major = [[versionArray objectAtIndex:0] intValue];
    osver_minor = [[versionArray objectAtIndex:1] intValue];
    osver_patch = [[versionArray objectAtIndex:2] intValue];
#endif
}

inline bool _isAtLeastSystemVersion(unsigned major, unsigned minor, unsigned patch)
{
    initializeSystemVersion();
#define OSVER_TO_NUM(major, minor, patch) ((major << 20) + (minor << 10) + (patch))
#define OSVER_SYS() OSVER_TO_NUM(osver_major, osver_minor, osver_patch)
    if (OSVER_SYS() < OSVER_TO_NUM(major, minor, patch)) {
        return false;
    }
#undef OSVER_SYS
#undef OSVER_TO_NUM
    return true;
}
// compile statically
template<unsigned major, unsigned minor, unsigned patch>
inline bool isAtLeastSystemVersion()
{
    return _isAtLeastSystemVersion(major, minor, patch);
}
} // anonymous namespace

void getSystemVersion(unsigned *major, unsigned *minor, unsigned *patch) {
    initializeSystemVersion();
    *major = osver_major;
    *minor = osver_minor;
    *patch = osver_patch;
}

bool isAtLeastSystemVersion(unsigned major, unsigned minor, unsigned patch)
{
    return _isAtLeastSystemVersion(major, minor, patch);
}

bool isOSXYosemiteOrGreater()
{
    return isAtLeastSystemVersion<10, 10, 0>();
}

bool isOSXMavericksOrGreater()
{
    return isAtLeastSystemVersion<10, 9, 0>();
}

bool isOSXMountainLionOrGreater()
{
    return isAtLeastSystemVersion<10, 8, 0>();
}

bool isOSXLionOrGreater()
{
    return isAtLeastSystemVersion<10, 7, 0>();
}

} // namespace mac
} // namesapce utils

// ****************************************************************************
// darkmode related
// ****************************************************************************
@interface DarkmodeHelper : NSObject
- (void)getDarkMode;
@end

static bool darkMode = false;
static DarkModeChangedCallback *darkModeWatcher = NULL;
@implementation DarkmodeHelper
- (id) init {
    self = [super init];

    // darkmode is available version >= 10.10
    if (utils::mac::isOSXYosemiteOrGreater()) {
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

// ****************************************************************************
// others
// ****************************************************************************
namespace utils {
namespace mac {

// another solution: hide dock icon when mainwindows is closed and show when
// mainwindows is shown
// http://stackoverflow.com/questions/16994331/multiprocessing-qt-app-how-can-i-limit-it-to-a-single-icon-in-the-macos-x-dock
void setDockIconStyle(bool hidden) {
    ProcessSerialNumber psn = { 0, kCurrentProcess };
    OSStatus err;
    if (hidden) {
        // kProcessTransformToBackgroundApplication is not support on OSX 10.7 and before
        // kProcessTransformToUIElementApplication is used for better fit when possible
        unsigned major;
        unsigned minor;
        unsigned patch;
        getSystemVersion(&major, &minor, &patch);
        if (major == 10 && minor == 7)
            err = TransformProcessType(&psn, kProcessTransformToBackgroundApplication);
        else
            err = TransformProcessType(&psn, kProcessTransformToUIElementApplication);
    } else {
        // kProcessTransformToForegroundApplication is supported on OSX 10.6 or later
        err = TransformProcessType(&psn, kProcessTransformToForegroundApplication);
    }
    if (err != noErr)
        qWarning("setDockIconStyle %s failure, status code: %d\n", (hidden ? "hidden" : "show"), err);
}

void orderFrontRegardless(unsigned long long win_id, bool force) {
    NSView __weak *widget =  (__bridge NSView*)(void*)win_id;
    NSWindow *window = [widget window];
    if(force || [window isVisible])
        [window performSelector:@selector(orderFrontRegardless) withObject:nil afterDelay:0.05];
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
    CFURLRef URLToToggle = (__bridge CFURLRef)itemURL;

    bool found = false;

    LSSharedFileListRef loginItems = LSSharedFileListCreate(NULL, kLSSharedFileListSessionLoginItems, NULL);
    if (loginItems) {
        UInt32 seed = 0U;
        CFArrayRef currentLoginItems = LSSharedFileListCopySnapshot(loginItems,
                                                                    &seed);
        const CFIndex count = CFArrayGetCount(currentLoginItems);
        for (CFIndex idx = 0; idx < count; ++idx) {
            LSSharedFileListItemRef item = (LSSharedFileListItemRef)CFArrayGetValueAtIndex(currentLoginItems, idx);
            CFURLRef outURL = NULL;

            const UInt32 resolutionFlags = kLSSharedFileListNoUserInteraction | kLSSharedFileListDoNotMountVolumes;
#if (__MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_10)
            outURL = LSSharedFileListItemCopyResolvedURL(item, resolutionFlags, /*outError*/ NULL);
            if (outURL == NULL) {
#else
            OSStatus err = LSSharedFileListItemResolve(item, resolutionFlags, &outURL, /*outRef*/ NULL);
            if (err != noErr || outURL == NULL) {
#endif
                if (outURL)
                    CFRelease(outURL);
                continue;
            }
            found = CFEqual(outURL, URLToToggle);
            CFRelease(outURL);

            if (found)
                break;
        }
        CFRelease(currentLoginItems);
        CFRelease(loginItems);
    }
    return found;
}

void set_auto_start(bool enabled)
{
    NSURL *itemURL = [[NSBundle mainBundle] bundleURL];
    CFURLRef URLToToggle = (__bridge CFURLRef)itemURL;

    LSSharedFileListRef loginItems = LSSharedFileListCreate(kCFAllocatorDefault, kLSSharedFileListSessionLoginItems, /*options*/ NULL);
    if (loginItems) {
        UInt32 seed = 0U;
        Boolean found;
        LSSharedFileListItemRef existingItem = NULL;

        CFArrayRef currentLoginItems = LSSharedFileListCopySnapshot(loginItems,
                                                                    &seed);
        const CFIndex count = CFArrayGetCount(currentLoginItems);
        for (CFIndex idx = 0; idx < count; ++idx) {
            LSSharedFileListItemRef item = (LSSharedFileListItemRef)CFArrayGetValueAtIndex(currentLoginItems, idx);
            CFURLRef outURL = NULL;

            const UInt32 resolutionFlags = kLSSharedFileListNoUserInteraction | kLSSharedFileListDoNotMountVolumes;
#if (__MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_10)
            outURL = LSSharedFileListItemCopyResolvedURL(item, resolutionFlags, /*outError*/ NULL);
            if (outURL == NULL) {
#else
            OSStatus err = LSSharedFileListItemResolve(item, resolutionFlags, &outURL, /*outRef*/ NULL);
            if (err != noErr || outURL == NULL) {
#endif
                if (outURL)
                    CFRelease(outURL);
                continue;
            }
            found = CFEqual(outURL, URLToToggle);
            CFRelease(outURL);

            if (found) {
                existingItem = item;
                break;
            }
        }

        if (enabled && !found) {
            NSString *displayName = @"Horizonbase";
            IconRef icon = NULL;
            FSRef ref;
            // TODO: replace the deprecated CFURLGetFSRef
            Boolean gotRef = CFURLGetFSRef(URLToToggle, &ref);
            if (gotRef) {
                OSStatus err = GetIconRefFromFileInfo(
                    &ref,
                    /*fileNameLength*/ 0,
                    /*fileName*/ NULL, kFSCatInfoNone,
                    /*catalogInfo*/ NULL, kIconServicesNormalUsageFlag, &icon,
                    /*outLabel*/ NULL);
                if (err != noErr) {
                    if (icon)
                        CFRelease(icon);
                    icon = NULL;
                }
            }

            LSSharedFileListItemRef newItem = LSSharedFileListInsertItemURL(
                loginItems, kLSSharedFileListItemBeforeFirst,
                (__bridge CFStringRef)displayName, icon, URLToToggle,
                /*propertiesToSet*/ NULL, /*propertiesToClear*/ NULL);
            if (newItem)
                CFRelease(newItem);
            if (icon)
                CFRelease(icon);
        } else if (!enabled && found) {
            LSSharedFileListItemRemove(loginItems, existingItem);
        }

        CFRelease(currentLoginItems);
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

void copyTextToPasteboard(const QString &text) {
    NSString *text_data = [NSString stringWithUTF8String:text.toUtf8().data()];
    NSPasteboard *paste_board = [NSPasteboard generalPasteboard];
    [paste_board clearContents];
    [paste_board declareTypes:[NSArray arrayWithObjects:NSStringPboardType, nil] owner:nil];
    [paste_board writeObjects:@[text_data]];
}

QString mainBundlePath() {
    NSURL *url = [[NSBundle mainBundle] bundleURL];
    return [[url path] UTF8String];
}

static inline bool isSslPolicy(SecPolicyRef policy) {
    bool is_ssl = false;
    CFDictionaryRef properties = NULL;
    if (!policy)
        return false;
    if ((properties = SecPolicyCopyProperties(policy)) == NULL)
        return false;
    CFTypeRef value = NULL;
    if (CFDictionaryGetValueIfPresent(properties, kSecPolicyOid,
                                      (const void **)&value) &&
        CFEqual(value, kSecPolicyAppleSSL))
        is_ssl = true;

    ;
    CFRelease(properties);
    return is_ssl;
}

static bool isCertificateDistrustedByUser(SecCertificateRef cert,
                                          SecTrustSettingsDomain domain) {
    CFArrayRef trustSettings;
    // On return, an array of CFDictionary objects specifying the trust settings
    // for the certificate
    OSStatus status = SecTrustSettingsCopyTrustSettings(cert, domain, &trustSettings);
    if (status != errSecSuccess)
        return false;

    bool distrusted = false;

    CFNumberRef result;
    SecTrustSettingsResult result_val;
    CFIndex size = CFArrayGetCount(trustSettings);
    for (CFIndex i = 0; i < size; ++i) {
        CFDictionaryRef trustSetting = (CFDictionaryRef)CFArrayGetValueAtIndex(trustSettings, i);
        SecPolicyRef policy = (SecPolicyRef)CFDictionaryGetValue(trustSetting, kSecTrustSettingsPolicy);

        if (isSslPolicy(policy) &&
            CFDictionaryGetValueIfPresent(trustSetting, kSecTrustSettingsResult,
                                          (const void **)&result)) {
            if (!CFNumberGetValue(result, kCFNumberIntType, &result_val))
                continue;
            switch (result_val) {
            case kSecTrustSettingsResultTrustRoot:
            case kSecTrustSettingsResultTrustAsRoot:
            case kSecTrustSettingsResultUnspecified:
                distrusted = false;
                break;
            case kSecTrustSettingsResultInvalid:
            case kSecTrustSettingsResultDeny:
            default:
                distrusted = true;
                break;
            }

            break;
        }
    }

    CFRelease(trustSettings);

    return distrusted;
}

static void
appendCaCertificateFromSecurityStore(std::vector<QByteArray> *retval,
                                     SecTrustSettingsDomain domain) {
    CFArrayRef certs;
    OSStatus status = 1;
    status = SecTrustSettingsCopyCertificates(domain, &certs);
    if (status != errSecSuccess)
        return;

    CFIndex size = CFArrayGetCount(certs);
    for (CFIndex i = 0; i < size; ++i) {
        SecCertificateRef cert =
            (SecCertificateRef)CFArrayGetValueAtIndex(certs, i);

        if (isCertificateDistrustedByUser(cert, kSecTrustSettingsDomainSystem) ||
            isCertificateDistrustedByUser(cert, kSecTrustSettingsDomainAdmin) ||
            isCertificateDistrustedByUser(cert, kSecTrustSettingsDomainUser)) {
            CFStringRef name;
            status = SecCertificateCopyCommonName(cert, &name);
            if (status == errSecSuccess && name != nil) {
                qWarning("declining a distrusted CA certificate from the system"
                         "store with common name %s",
                         [(__bridge NSString*)name UTF8String]);
                CFRelease(name);
            }
            else
                qWarning("declining a distrusted CA certificate from the system store");
            continue;
        }

        // copy if trusted
        CFDataRef data;
        data = SecCertificateCopyData(cert);

        if (data == NULL) {
            qWarning("error retrieving a CA certificate from the system store");
        } else {
            QByteArray raw_data((const char *)CFDataGetBytePtr(data), CFDataGetLength(data));
            retval->push_back(raw_data);
            CFRelease(data);
        }
    }
    CFRelease(certs);
}

std::vector<QByteArray> getSystemCaCertificates() {
    std::vector<QByteArray> retval;
    appendCaCertificateFromSecurityStore(&retval, kSecTrustSettingsDomainSystem);
    appendCaCertificateFromSecurityStore(&retval, kSecTrustSettingsDomainAdmin);
    appendCaCertificateFromSecurityStore(&retval, kSecTrustSettingsDomainUser);
    return retval;
}

} // namespace mac
} // namespace utils
