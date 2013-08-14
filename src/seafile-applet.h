#ifndef SEAFILE_CLIENT_APPLET_H
#define SEAFILE_CLIENT_APPLET_H

#include <QObject>

class Configurator;
class DaemonManager;
class RpcClient;
class AccountManager;
class MainWindow;
class QAction;
class QIcon;
class QSystemTrayIcon;


enum TrayState {
    TRAY_DAEMON_UP = 0,
    TRAY_DAEMON_DOWN,
    TRAY_DAEMON_AUTOSYNC_DISABLED,
    TRAY_TRANSFER_1,
    TRAY_TRANSFER_2,
    TRAY_TRANSFER_3,
    TRAY_TRANSFER_4,
};

/**
 * The central class of seafile-client
 */
class SeafileApplet : QObject {
    Q_OBJECT

public:
    SeafileApplet();

    void start();

    // normal exit
    void exit(int code);

    // Show error in a messagebox and exit
    void errorAndExit(const QString& error);

    // accessors
    AccountManager *accountManager() { return account_mgr_; }
    RpcClient *rpcClient() { return rpc_client_; }
    DaemonManager *daemonManager() { return daemon_mgr_; }
    Configurator *configurator() { return configurator_; }
    MainWindow *mainWindow() { return main_win_; }
    QIcon getIcon(TrayState state);
    void setTrayState(TrayState);

private slots:
    void onCcnetDaemonConnected();
    void onCcnetDaemonDisconnected();
    void openAdmin();
    void disableAutoSync();
    void enableAutoSync();
    void restartSeafile();
    void quitSeafile();

private:
    Q_DISABLE_COPY(SeafileApplet)

    Configurator *configurator_;
    DaemonManager *daemon_mgr_;
    MainWindow* main_win_;
    RpcClient *rpc_client_;
    AccountManager *account_mgr_;
    QSystemTrayIcon *systemTray;
    QAction *disableAutoSyncAction_;
    QAction *enableAutoSyncAction_;
    bool autoSync_;
};

/**
 * The global SeafileApplet object
 */
extern SeafileApplet *seafApplet;


#endif // SEAFILE_CLIENT_APPLET_H
