#ifndef SEAFILE_CLIENT_TRAY_ICON_H
#define SEAFILE_CLIENT_TRAY_ICON_H

#include <QSystemTrayIcon>
#include <QHash>

class QAction;
class QMenu;
#if defined(Q_WS_MAC)
class TrayNotificationManager;
#endif

class SeafileTrayIcon : public QSystemTrayIcon {
    Q_OBJECT

public:
    explicit SeafileTrayIcon(QObject *parent=0);

    enum TrayState {
        STATE_DAEMON_UP = 0,
        STATE_DAEMON_DOWN,
        STATE_DAEMON_AUTOSYNC_DISABLED,
        STATE_TRANSFER_1,
        STATE_TRANSFER_2,
        STATE_SERVERS_NOT_CONNECTED,
    };

    void start();

    void setState(TrayState);
    void notify(const QString &title, const QString &content);
    void rotate(bool start);

public slots:
    void showSettingsWindow();

private slots:
    void disableAutoSync();
    void enableAutoSync();
    void quitSeafile();
    void onActivated(QSystemTrayIcon::ActivationReason);
    void prepareContextMenu();
    void toggleMainWindow();
    void rotateTrayIcon();
    void refreshTrayIcon();
    void openHelp();
    void about();

private:
    Q_DISABLE_COPY(SeafileTrayIcon)

    void createActions();
    void createContextMenu();
    bool allServersConnected();

    QIcon stateToIcon(TrayState state);
    QIcon getIcon(const QString& name);
    void resetToolTip();

    QMenu *context_menu_;
    QMenu *help_menu_;

    // Actions for tray icon menu
    QAction *enable_auto_sync_action_;
    QAction *disable_auto_sync_action_;
    QAction *quit_action_;
    QAction *toggle_main_window_action_;
    QAction *settings_action_;

    QAction *about_action_;
    QAction *open_help_action_;

#if defined(Q_WS_MAC)
    TrayNotificationManager *tnm;
#endif

    QTimer *rotate_timer_;
    QTimer *refresh_timer_;
    int nth_trayicon_;
    int rotate_counter_;
    bool auto_sync_;

    TrayState state_;

    QHash<QString, QIcon> icon_cache_;
};

#endif // SEAFILE_CLIENT_TRAY_ICON_H

