#include <QStringList>
#include <QStringRef>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QDebug>
#include "finder-sync/finder-sync.h"
#include "utils/utils.h"
#include "utils/utils-mac.h"

static const char* kApplePluginkitBinary = "/usr/bin/pluginkit";
static const char* kFinderSyncBundleIdentifier = "com.seafile.seafile-client.findersync";

// run command and arugments,
// and return the termination status
// if we have non-null output, we will write stdout (not stderr) output to it
static int runAsCommand(const QString &binary, const QStringList &arguments, QString
                 *output = nullptr) {
    QProcess process;
    process.start(binary, arguments);
    if (!process.waitForFinished(500))
        return false;
    if (output)
        *output = process.readAllStandardOutput().trimmed();
    return process.exitCode();
}

static inline QString pluginPath() {
#ifdef XCODE_APP
    return QDir(utils::mac::mainBundlePath()).filePath("Contents/PlugIns/Seafile FinderSync.appex");
#else
    return QDir(utils::mac::mainBundlePath()).filePath("fsplugin/Seafile FinderSync.appex");
#endif
}

/// \brief list all installed plugins
static bool installedPluginPath(QString *path) {
    QStringList arguments {"-m", "-v", "-i", kFinderSyncBundleIdentifier};
    int status = runAsCommand(kApplePluginkitBinary, arguments, path);
    if (status != 0 || path->isEmpty())
        return false;
    int begin = path->indexOf('/');
    if (begin == -1)
        return false;
    int end = path->indexOf('\n', begin);
    if (end == -1 || end - begin < 0)
        return false;
    *path = QStringRef(path, begin, end - begin).toString();

    qDebug("[FinderSync] found installed plugin %s", path->toUtf8().data());

    return true;
}

bool FinderSyncExtensionHelper::isInstalled() {
    QString output;
    QStringList arguments {"-m", "-i", kFinderSyncBundleIdentifier};
    int status = runAsCommand(kApplePluginkitBinary, arguments, &output);
    if (status != 0 || output.isEmpty())
        return false;

    return true;
}

bool FinderSyncExtensionHelper::isEnabled() {
    QString output;
    QStringList arguments {"-m", "-i", kFinderSyncBundleIdentifier};
    int status = runAsCommand(kApplePluginkitBinary, arguments, &output);
    if (status != 0 || output.isEmpty())
        return false;
    if (output[0] != '+' && output[0] != '?')
        return false;

    return true;
}

bool FinderSyncExtensionHelper::isBundled() {
    QString plugin_path = pluginPath();
    if (!QFileInfo(plugin_path).isDir()) {
        qDebug("[FinderSync] unable to find bundled plugin at %s", plugin_path.toUtf8().data());
        return false;
    }

    qDebug("[FinderSync] found bundled plugin at %s", plugin_path.toUtf8().data());
    return true;
}

// Developer notes:
//  In Mac OSX Sierra and higher, to install a finder plugin (a .appex folder) ,
//  two conditions must be satisfied:
//    - the plugin must be signed
//    - the plugin must be included as part of a .app
//  So when seadrive-gui is not compiled with xcode, there is no way to install
//  the plugin.
#ifdef XCODE_APP
bool FinderSyncExtensionHelper::reinstall(bool force) {
    if (!isBundled())
        return false;

    QString bundled_plugin_path = pluginPath();
    QString plugin_path;
    QStringList remove_arguments;

    // remove all installed plugins
    while(installedPluginPath(&plugin_path)) {
        if (!force && bundled_plugin_path == plugin_path) {
            qDebug("[FinderSync] current plugin detected: %s",
                   bundled_plugin_path.toUtf8().data());
            return true;
        }
        remove_arguments = QStringList {"-r", plugin_path};
        // this command returns non-zero when succeeds,
        // so don't bother to check it
        runAsCommand(kApplePluginkitBinary, remove_arguments);
    }

    QStringList install_arguments {"-a", bundled_plugin_path};
    int status = runAsCommand(kApplePluginkitBinary, install_arguments);
    if (status != 0)
        return false;

    qWarning("[FinderSync] reinstalled");
    return true;
}
#else
bool FinderSyncExtensionHelper::reinstall(bool force) {
    return false;
}
#endif

bool FinderSyncExtensionHelper::setEnable(bool enable) {
    const char *election = enable ? "use" : "ignore";
    QStringList arguments {"-e", election, "-i", kFinderSyncBundleIdentifier};
    int status = runAsCommand(kApplePluginkitBinary, arguments);
    if (status != 0)
        return false;

    return true;
}
