#include <QSettings>
#include <QHostInfo>
#include <QNetworkProxy>
#include <QNetworkProxyQuery>
#include "utils/utils.h"
#include "utils/utils-mac.h"
#include "seafile-applet.h"
#include "ui/tray-icon.h"
#include "settings-mgr.h"
#include "rpc/rpc-client.h"
#include "utils/utils.h"
#include "network-mgr.h"
#include "ui/main-window.h"

#if defined(Q_OS_WIN32)
#include "utils/registry.h"
#endif

#ifdef HAVE_FINDER_SYNC_SUPPORT
#include "finder-sync/finder-sync.h"
#endif

namespace
{
const char *kHideMainWindowWhenStarted = "hideMainWindowWhenStarted";
const char *kHideDockIcon = "hideDockIcon";
const char *kCheckLatestVersion = "checkLatestVersion";
const char *kEnableSyncingWithExistingFolder = "syncingWithExistingFolder";
const char *kBehaviorGroup = "Behavior";

// const char *kDefaultLibraryAlreadySetup = "defaultLibraryAlreadySetup";
// const char *kStatusGroup = "Status";

const char *kSettingsGroup = "Settings";
const char *kComputerName = "computerName";
#ifdef HAVE_FINDER_SYNC_SUPPORT
const char *kFinderSync = "finderSync";
#endif // HAVE_FINDER_SYNC_SUPPORT
#ifdef HAVE_SHIBBOLETH_SUPPORT
const char *kLastShibUrl = "lastShiburl";
#endif // HAVE_SHIBBOLETH_SUPPORT

const char *kUseProxy = "use_proxy";
const char *kProxyType = "proxy_type";
const char *kProxyAddr = "proxy_addr";
const char *kProxyPort = "proxy_port";
const char *kProxyUsername = "proxy_username";
const char *kProxyPassword = "proxy_password";


#ifdef Q_OS_WIN32
QString softwareSeafile()
{
    return QString("SOFTWARE\\%1").arg(getBrand());
}
#endif

} // namespace


SettingsManager::SettingsManager()
    : auto_sync_(true),
      bubbleNotifycation_(true),
      autoStart_(false),
      transferEncrypted_(true),
      allow_invalid_worktree_(false),
      allow_repo_not_found_on_server_(false),
      sync_extra_temp_file_(false),
      maxDownloadRatio_(0),
      maxUploadRatio_(0),
      verify_http_sync_cert_disabled_(false),
      current_proxy_(SeafileProxy())
{
}

void SettingsManager::loadSettings()
{
    QString str;
    int value;

    if (seafApplet->rpcClient()->seafileGetConfig("notify_sync", &str) >= 0)
        bubbleNotifycation_ = (str == "off") ? false : true;

    if (seafApplet->rpcClient()->ccnetGetConfig("encrypt_channel", &str) >= 0)
        transferEncrypted_ = (str == "off") ? false : true;

    if (seafApplet->rpcClient()->seafileGetConfigInt("download_limit",
                                                     &value) >= 0)
        maxDownloadRatio_ = value >> 10;

    if (seafApplet->rpcClient()->seafileGetConfigInt("upload_limit", &value) >=
        0)
        maxUploadRatio_ = value >> 10;

    if (seafApplet->rpcClient()->seafileGetConfig("allow_invalid_worktree",
                                                  &str) >= 0)
        allow_invalid_worktree_ = (str == "true") ? true : false;

    if (seafApplet->rpcClient()->seafileGetConfig("sync_extra_temp_file",
                                                  &str) >= 0)
        sync_extra_temp_file_ = (str == "true") ? true : false;

    if (seafApplet->rpcClient()->seafileGetConfig(
            "allow_repo_not_found_on_server", &str) >= 0)
        allow_repo_not_found_on_server_ = (str == "true") ? true : false;

    if (seafApplet->rpcClient()->seafileGetConfig("disable_verify_certificate",
                                                  &str) >= 0)
        verify_http_sync_cert_disabled_ = (str == "true") ? true : false;

    loadProxySettings();

    autoStart_ = get_seafile_auto_start();

#ifdef HAVE_FINDER_SYNC_SUPPORT
    // try to do a reinstall, or we may use findersync somewhere else
    // this action won't stop findersync if running already
    FinderSyncExtensionHelper::reinstall();

    // try to sync finder sync extension settings with the actual settings
    // i.e. enabling the finder sync if the setting is true
    setFinderSyncExtension(getFinderSyncExtension());
#endif // HAVE_FINDER_SYNC_SUPPORT


#ifdef Q_OS_WIN32
    RegElement reg(HKEY_CURRENT_USER, softwareSeafile(), "ShellExtDisabled",
                   "");
    shell_ext_enabled_ = !reg.exists();
#endif
}

void SettingsManager::loadProxySettings()
{
    SeafileProxy proxy;

    QString use_proxy;
    seafApplet->rpcClient()->seafileGetConfig(kUseProxy, &use_proxy);
    if (use_proxy != "true") {
        return;
    }

    QString proxy_type;
    QString proxy_host;
    int proxy_port;
    QString proxy_username;
    QString proxy_password;

    if (seafApplet->rpcClient()->seafileGetConfig(kProxyAddr, &proxy_host) <
        0) {
        return;
    }
    if (seafApplet->rpcClient()->seafileGetConfigInt(kProxyPort, &proxy_port) <
        0) {
        return;
    }
    if (seafApplet->rpcClient()->seafileGetConfig(kProxyType, &proxy_type) <
        0) {
        return;
    }
    if (proxy_type == "http") {
        if (seafApplet->rpcClient()->seafileGetConfig(kProxyUsername,
                                                      &proxy_username) < 0) {
            return;
        }
        if (seafApplet->rpcClient()->seafileGetConfig(kProxyPassword,
                                                      &proxy_password) < 0) {
            return;
        }
        proxy.type = HttpProxy;
        proxy.host = proxy_host;
        proxy.port = proxy_port;
        proxy.username = proxy_username;
        proxy.password = proxy_password;

    } else if (proxy_type == "socks") {
        proxy.type = SocksProxy;
        proxy.host = proxy_host;
        proxy.port = proxy_port;

    } else if (proxy_type == "system") {
        proxy.type = SystemProxy;
    } else if (!proxy_type.isEmpty()) {
        qWarning("Unsupported proxy_type %s", proxy_type.toUtf8().data());
        return;
    }

    current_proxy_ = proxy;
    applyProxySettings();
}


void SettingsManager::setAutoSync(bool auto_sync)
{
    if (seafApplet->rpcClient()->setAutoSync(auto_sync) < 0) {
        // Error
        return;
    }
    auto_sync_ = auto_sync;
    seafApplet->trayIcon()->setState(
        auto_sync ? SeafileTrayIcon::STATE_DAEMON_UP
                  : SeafileTrayIcon::STATE_DAEMON_AUTOSYNC_DISABLED);
    emit autoSyncChanged(auto_sync);
}

void SettingsManager::setNotify(bool notify)
{
    if (bubbleNotifycation_ != notify) {
        if (seafApplet->rpcClient()->seafileSetConfig(
                "notify_sync", notify ? "on" : "off") < 0) {
            // Error
            return;
        }
        bubbleNotifycation_ = notify;
    }
}

void SettingsManager::setAutoStart(bool autoStart)
{
    if (autoStart_ != autoStart) {
        if (set_seafile_auto_start(autoStart) >= 0)
            autoStart_ = autoStart;
    }
}

void SettingsManager::setEncryptTransfer(bool encrypted)
{
    if (transferEncrypted_ != encrypted) {
        if (seafApplet->rpcClient()->ccnetSetConfig(
                "encrypt_channel", encrypted ? "on" : "off") < 0) {
            // Error
            return;
        }
        transferEncrypted_ = encrypted;
    }
}

void SettingsManager::setMaxDownloadRatio(unsigned int ratio)
{
    if (maxDownloadRatio_ != ratio) {
        if (seafApplet->rpcClient()->setDownloadRateLimit(ratio << 10) < 0) {
            // Error
            return;
        }
        maxDownloadRatio_ = ratio;
    }
}

void SettingsManager::setMaxUploadRatio(unsigned int ratio)
{
    if (maxUploadRatio_ != ratio) {
        if (seafApplet->rpcClient()->setUploadRateLimit(ratio << 10) < 0) {
            // Error
            return;
        }
        maxUploadRatio_ = ratio;
    }
}

bool SettingsManager::hideMainWindowWhenStarted()
{
    QSettings settings;
    bool hide;

    settings.beginGroup(kBehaviorGroup);
    hide = settings.value(kHideMainWindowWhenStarted, false).toBool();
    settings.endGroup();

    return hide;
}

void SettingsManager::setHideMainWindowWhenStarted(bool hide)
{
    QSettings settings;

    settings.beginGroup(kBehaviorGroup);
    settings.setValue(kHideMainWindowWhenStarted, hide);
    settings.endGroup();
}

bool SettingsManager::hideDockIcon()
{
    QSettings settings;
    bool hide;

    settings.beginGroup(kBehaviorGroup);
    hide = settings.value(kHideDockIcon, false).toBool();
    settings.endGroup();
    return hide;
}

void SettingsManager::setHideDockIcon(bool hide)
{
    QSettings settings;

    settings.beginGroup(kBehaviorGroup);
    settings.setValue(kHideDockIcon, hide);
    settings.endGroup();

    set_seafile_dock_icon_style(hide);
#ifdef Q_OS_MAC
    // for UIElement application, the main window might sink
    // under many applications
    // this will force it to stand before all
    utils::mac::orderFrontRegardless(seafApplet->mainWindow()->winId());
#endif
}

// void SettingsManager::setDefaultLibraryAlreadySetup()
// {
//     QSettings settings;

//     settings.beginGroup(kStatusGroup);
//     settings.setValue(kDefaultLibraryAlreadySetup, true);
//     settings.endGroup();
// }


// bool SettingsManager::defaultLibraryAlreadySetup()
// {
//     QSettings settings;
//     bool done;

//     settings.beginGroup(kStatusGroup);
//     done = settings.value(kDefaultLibraryAlreadySetup, false).toBool();
//     settings.endGroup();

//     return done;
// }

void SettingsManager::removeAllSettings()
{
    QSettings settings;
    settings.clear();

#if defined(Q_OS_WIN32)
    RegElement::removeRegKey(HKEY_CURRENT_USER, "SOFTWARE", getBrand());
#endif
}

void SettingsManager::setCheckLatestVersionEnabled(bool enabled)
{
    QSettings settings;

    settings.beginGroup(kBehaviorGroup);
    settings.setValue(kCheckLatestVersion, enabled);
    settings.endGroup();
}

bool SettingsManager::isCheckLatestVersionEnabled()
{
    QString brand = getBrand();

    if (brand != "Seafile") {
        return false;
    }

    QSettings settings;
    bool enabled;

    settings.beginGroup(kBehaviorGroup);
    enabled = settings.value(kCheckLatestVersion, true).toBool();
    settings.endGroup();

    return enabled;
}

void SettingsManager::setAllowInvalidWorktree(bool val)
{
    if (allow_invalid_worktree_ != val) {
        if (seafApplet->rpcClient()->seafileSetConfig(
                "allow_invalid_worktree", val ? "true" : "false") < 0) {
            // Error
            return;
        }
        allow_invalid_worktree_ = val;
    }
}

void SettingsManager::setSyncExtraTempFile(bool sync)
{
    if (sync_extra_temp_file_ != sync) {
        if (seafApplet->rpcClient()->seafileSetConfig(
                "sync_extra_temp_file", sync ? "true" : "false") < 0) {
            // Error
            return;
        }
        sync_extra_temp_file_ = sync;
    }
}

void SettingsManager::getProxy(QNetworkProxy *proxy) const
{
    current_proxy_.toQtNetworkProxy(proxy);
    return;
}

void SettingsManager::SeafileProxy::toQtNetworkProxy(QNetworkProxy *proxy) const
{
    if (type == NoProxy) {
        proxy->setType(QNetworkProxy::NoProxy);
        return;
    }
    proxy->setType(type == HttpProxy ? QNetworkProxy::HttpProxy
                                     : QNetworkProxy::Socks5Proxy);
    proxy->setHostName(host);
    proxy->setPort(port);
    if (type == HttpProxy && !username.isEmpty() && !password.isEmpty()) {
        proxy->setUser(username);
        proxy->setPassword(password);
    }
}

bool SettingsManager::SeafileProxy::operator==(const SeafileProxy &rhs) const
{
    return type == rhs.type && host == rhs.host && port == rhs.port &&
           username == rhs.username && password == rhs.password;
}

void SettingsManager::setProxy(const SeafileProxy &proxy)
{
    if (proxy == current_proxy_) {
        return;
    }
    current_proxy_ = proxy;

    writeProxySettingsToDaemon(proxy);
    applyProxySettings();
}

void SettingsManager::applyProxySettings()
{
    if (current_proxy_.type == SystemProxy) {
        QNetworkProxyFactory::setUseSystemConfiguration(true);
        qDebug("Using system proxy: ON");
        return;
    } else {
        QNetworkProxyFactory::setUseSystemConfiguration(false);
        qDebug("Using system proxy: OFF");
    }

    if (current_proxy_.type == NoProxy) {
        QNetworkProxy::setApplicationProxy(QNetworkProxy::NoProxy);
    }

    QNetworkProxy proxy;
    getProxy(&proxy);
    NetworkManager::instance()->applyProxy(proxy);
}

void SettingsManager::writeProxySettingsToDaemon(const SeafileProxy &proxy)
{
    SeafileRpcClient *rpc = seafApplet->rpcClient();
    if (proxy.type == NoProxy) {
        rpc->seafileSetConfig(kUseProxy, "false");
        return;
    }

    if (rpc->seafileSetConfig(kUseProxy, "true") < 0) {
        return;
    }

    if (proxy.type == SystemProxy) {
        rpc->seafileSetConfig(kProxyType, "system");
        return;
    }

    if (rpc->seafileSetConfig(kProxyType,
                              proxy.type == HttpProxy ? "http" : "socks") < 0)
        return;
    if (rpc->seafileSetConfig(kProxyAddr, proxy.host.toUtf8().data()) < 0)
        return;
    if (rpc->seafileSetConfigInt(kProxyPort, proxy.port) < 0)
        return;
    if (proxy.type == HttpProxy) {
        if (rpc->seafileSetConfig(kProxyUsername,
                                  proxy.username.toUtf8().data()) < 0)
            return;
        if (rpc->seafileSetConfig(kProxyPassword,
                                  proxy.password.toUtf8().data()) < 0)
            return;
    }
}

void SettingsManager::setAllowRepoNotFoundOnServer(bool val)
{
    if (allow_repo_not_found_on_server_ != val) {
        if (seafApplet->rpcClient()->seafileSetConfig(
                "allow_repo_not_found_on_server", val ? "true" : "false") < 0) {
            // Error
            return;
        }
        allow_repo_not_found_on_server_ = val;
    }
}

void SettingsManager::setHttpSyncCertVerifyDisabled(bool disabled)
{
    if (verify_http_sync_cert_disabled_ != disabled) {
        if (seafApplet->rpcClient()->seafileSetConfig(
                "disable_verify_certificate", disabled ? "true" : "false") <
            0) {
            // Error
            return;
        }
        verify_http_sync_cert_disabled_ = disabled;
    }
}

bool SettingsManager::isEnableSyncingWithExistingFolder() const
{
    bool enabled;
    QSettings settings;

    settings.beginGroup(kBehaviorGroup);
    enabled = settings.value(kEnableSyncingWithExistingFolder, false).toBool();
    settings.endGroup();

    return enabled;
}

void SettingsManager::setEnableSyncingWithExistingFolder(bool enabled)
{
    QSettings settings;

    settings.beginGroup(kBehaviorGroup);
    settings.setValue(kEnableSyncingWithExistingFolder, enabled);
    settings.endGroup();
}

QString SettingsManager::getComputerName()
{
    QSettings settings;
    QString name;

    QString default_computer_Name = QHostInfo::localHostName();

    settings.beginGroup(kSettingsGroup);
    name = settings.value(kComputerName, default_computer_Name).toString();
    settings.endGroup();

    return name;
}

void SettingsManager::setComputerName(const QString &computerName)
{
    QSettings settings;
    settings.beginGroup(kSettingsGroup);
    settings.setValue(kComputerName, computerName);
    settings.endGroup();
}

#ifdef HAVE_SHIBBOLETH_SUPPORT
QString SettingsManager::getLastShibUrl()
{
    QSettings settings;
    QString url;

    settings.beginGroup(kSettingsGroup);
    url = settings.value(kLastShibUrl, "").toString();
    settings.endGroup();

    return url;
}

void SettingsManager::setLastShibUrl(const QString &url)
{
    QSettings settings;
    settings.beginGroup(kSettingsGroup);
    settings.setValue(kLastShibUrl, url);
    settings.endGroup();
}
#endif // HAVE_SHIBBOLETH_SUPPORT

#ifdef HAVE_FINDER_SYNC_SUPPORT
bool SettingsManager::getFinderSyncExtension() const
{
    QSettings settings;
    bool enabled;

    settings.beginGroup(kSettingsGroup);
    enabled = settings.value(kFinderSync, true).toBool();
    settings.endGroup();

    return enabled;
}
bool SettingsManager::getFinderSyncExtensionAvailable() const
{
    return FinderSyncExtensionHelper::isInstalled();
}
void SettingsManager::setFinderSyncExtension(bool enabled)
{
    QSettings settings;

    settings.beginGroup(kSettingsGroup);
    settings.setValue(kFinderSync, enabled);
    settings.endGroup();

    // if setting operation fails
    if (!getFinderSyncExtensionAvailable()) {
        qWarning("Unable to find FinderSync Extension");
    } else if (enabled != FinderSyncExtensionHelper::isEnabled() &&
               !FinderSyncExtensionHelper::setEnable(enabled)) {
        qWarning("Unable to enable FinderSync Extension");
    }
}
#endif // HAVE_FINDER_SYNC_SUPPORT

#ifdef Q_OS_WIN32
void SettingsManager::setShellExtensionEnabled(bool enabled)
{
    shell_ext_enabled_ = enabled;

    RegElement reg1(HKEY_CURRENT_USER, softwareSeafile(), "", "");
    RegElement reg2(HKEY_CURRENT_USER, softwareSeafile(), "ShellExtDisabled",
                    "1");
    if (enabled) {
        reg2.remove();
    } else {
        reg1.add();
        reg2.add();
    }
}
#endif // Q_OS_WIN32

class UseSystemProxyContext {
public:
    UseSystemProxyContext() {
        QNetworkProxyFactory::setUseSystemConfiguration(true);
    }

    ~UseSystemProxyContext() {
        QNetworkProxyFactory::setUseSystemConfiguration(false);
    }
};

void SettingsManager::writeSystemProxyInfo(const QUrl &url,
                                           const QString &file_path)
{
    UseSystemProxyContext context;
    QNetworkProxyQuery query(url);
    bool use_proxy = true;
    QNetworkProxy proxy;
    QList<QNetworkProxy> proxies = QNetworkProxyFactory::proxyForQuery(query);

    // printf("list of proxies: %d\n", proxies.size());
    // foreach (const QNetworkProxy &proxy, proxies) {
    //     static int i = 0;
    //     printf("[proxy number %d] %d %s:%d %s %s \n", i++, (int)proxy.type(),
    //            proxy.hostName().toUtf8().data(), proxy.port(),
    //            proxy.user().toUtf8().data(),
    //            proxy.password().toUtf8().data());
    // }

    if (proxies.empty()) {
        use_proxy = false;
    } else {
        proxy = proxies[0];
        if (proxy.type() == QNetworkProxy::NoProxy ||
            proxy.type() == QNetworkProxy::DefaultProxy ||
            proxy.type() == QNetworkProxy::FtpCachingProxy) {
            use_proxy = false;
        }

        if (proxy.hostName().isEmpty() || proxy.port() == 0) {
            use_proxy = false;
        }
    }
    QString content;
    if (use_proxy) {
        QString type;
        if (proxy.type() == QNetworkProxy::HttpProxy ||
            proxy.type() == QNetworkProxy::HttpCachingProxy) {
            type = "http";
        } else {
            type = "socks";
        }
        QString json_content =
            "{\"type\": \"%1\", \"addr\": \"%2\", \"port\": %3, \"username\": "
            "\"%4\", \"password\": \"%5\"}";
        content = json_content.arg(type)
                      .arg(proxy.hostName())
                      .arg(proxy.port())
                      .arg(proxy.user())
                      .arg(proxy.password());
    } else {
        content = "{\"type\": \"none\"}";
    }

    QFile system_proxy_txt(file_path);
    if (!system_proxy_txt.open(QIODevice::WriteOnly)) {
        return;
    }

    system_proxy_txt.write(content.toUtf8().data());
}
