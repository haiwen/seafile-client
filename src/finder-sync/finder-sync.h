#ifndef SEAFILE_CLIENT_FINDER_SYNC_H_
#define SEAFILE_CLIENT_FINDER_SYNC_H_

class FinderSyncExtensionHelper {
public:
    /// \brief check if plugin is installed
    /// return true if installed
    ///
    static bool isInstalled();

    /// \brief check if plugin is enabled
    /// return true if enabled
    /// this value can be changed via preference by user
    static bool isEnabled();

    /// \brief check if plugin has been bundled
    /// return true if bundled
    ///
    static bool isBundled();

    /// \brief do reinstall for the bundled plugin
    /// return true if success
    ///
    static bool reinstall(bool force = false);

    /// \brief change the plugin's status
    /// return ture if success
    ///
    static bool setEnable(bool enable);
};

#endif // SEAFILE_CLIENT_FINDER_SYNC_H_
