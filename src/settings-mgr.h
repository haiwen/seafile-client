#ifndef SEAFILE_CLIENT_SETTINGS_MANAGER_H
#define SEAFILE_CLIENT_SETTINGS_MANAGER_H

#include <QObject>

/**
 * Settings Manager handles seafile client user settings & preferences
 */
class QNetworkProxy;
class QUrl;

class SettingsManager : public QObject {
    Q_OBJECT

public:
    enum ProxyType {
        NoProxy = 0,
        HttpProxy = 1,
        SocksProxy = 2,
        SystemProxy = 3
    };

    struct SeafileProxy {
        ProxyType type;

        QString host;
        int port;
        QString username;
        QString password;

        SeafileProxy(ProxyType _type = NoProxy,
                     const QString _host = QString(),
                     int _port = 0,
                     const QString& _username = QString(),
                     const QString& _password = QString())
            : type(_type),
              host(_host),
              port(_port),
              username(_username),
              password(_password)
        {
        }

        void toQtNetworkProxy(QNetworkProxy *proxy) const;

        bool operator==(const SeafileProxy& rhs) const;
        bool operator!=(const SeafileProxy& rhs) const { return !(*this == rhs); };
    };

    SettingsManager();

    void loadSettings();
    void setAutoSync(bool);

    bool autoSync() { return auto_sync_; }

    bool notify() { return bubbleNotifycation_; }
    bool autoStart() { return autoStart_; }
    bool encryptTransfer() { return transferEncrypted_; }
    unsigned int maxDownloadRatio() { return maxDownloadRatio_; }
    unsigned int maxUploadRatio() { return maxUploadRatio_; }
    bool allowInvalidWorktree() { return allow_invalid_worktree_; }
    bool syncExtraTempFile() { return sync_extra_temp_file_; }

    void getProxy(QNetworkProxy *proxy) const;
    SeafileProxy getProxy() const { return current_proxy_; };

    void setNotify(bool notify);
    void setAutoStart(bool autoStart);
    void setEncryptTransfer(bool encrypted);
    void setMaxDownloadRatio(unsigned int ratio);
    void setMaxUploadRatio(unsigned int ratio);
    void setAllowInvalidWorktree(bool val);
    void setSyncExtraTempFile(bool sync);
    void setProxy(const SeafileProxy& proxy);

    bool hideMainWindowWhenStarted();
    void setHideMainWindowWhenStarted(bool hide);

    bool hideDockIcon();
    void setHideDockIcon(bool hide);

    void setCheckLatestVersionEnabled(bool enabled);
    bool isCheckLatestVersionEnabled();
    // bool defaultLibraryAlreadySetup();
    // void setDefaultLibraryAlreadySetup();

    void setAllowRepoNotFoundOnServer(bool enabled);
    bool allowRepoNotFoundOnServer() const { return allow_repo_not_found_on_server_; };

    void setHttpSyncCertVerifyDisabled(bool disabled);
    bool httpSyncCertVerifyDisabled() const { return verify_http_sync_cert_disabled_; };

    QString getComputerName();
    void setComputerName(const QString& computerName);

    bool isEnableSyncingWithExistingFolder() const;
    void setEnableSyncingWithExistingFolder(bool enabled);

#ifdef HAVE_SHIBBOLETH_SUPPORT
    QString getLastShibUrl();
    void setLastShibUrl(const QString& url);
#endif // HAVE_SHIBBOLETH_SUPPORT

#ifdef HAVE_FINDER_SYNC_SUPPORT
    bool getFinderSyncExtension() const;
    bool getFinderSyncExtensionAvailable() const;
    void setFinderSyncExtension(bool enabled);
#endif // HAVE_FINDER_SYNC_SUPPORT

#ifdef Q_OS_WIN32
    void setShellExtensionEnabled(bool enabled);
    bool shellExtensionEnabled() const { return shell_ext_enabled_; }
#endif // HAVE_FINDER_SYNC_SUPPORT

public:

    // Remove all settings from system when uninstall
    static void removeAllSettings();
    // Write the system proxy information, to be read by seaf-daemon.
    static void writeSystemProxyInfo(const QUrl& url, const QString& file_path);

signals:
    void autoSyncChanged(bool auto_sync);

private:
    Q_DISABLE_COPY(SettingsManager)

    void loadProxySettings();
    void writeProxySettingsToDaemon(const SeafileProxy& proxy);
    void applyProxySettings();

    bool auto_sync_;
    bool bubbleNotifycation_;
    bool autoStart_;
    bool transferEncrypted_;
    bool allow_invalid_worktree_;
    bool allow_repo_not_found_on_server_;
    bool sync_extra_temp_file_;
    unsigned int maxDownloadRatio_;
    unsigned int maxUploadRatio_;
    bool verify_http_sync_cert_disabled_;
    bool shell_ext_enabled_;

    // proxy settings
    SeafileProxy current_proxy_;
};

#endif // SEAFILE_CLIENT_SETTINGS_MANAGER_H
