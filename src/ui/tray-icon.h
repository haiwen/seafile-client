#ifndef SEAFILE_CLIENT_TRAY_ICON_H
#define SEAFILE_CLIENT_TRAY_ICON_H

#include <QSystemTrayIcon>
#include <QHash>

class QAction;
class QMenu;
#if defined(Q_OS_MAC)
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
        STATE_HAVE_UNREAD_MESSAGE,
    };

    void start();

    void setState(TrayState state, const QString& tip=QString());
    void notify(const QString &title, const QString &content);
    void rotate(bool start);

    void showMessage(const QString & title, const QString & message, MessageIcon icon = Information, int millisecondsTimeoutHint = 10000);

public slots:
    void showSettingsWindow();

private slots:
    void disableAutoSync();
    void enableAutoSync();
    void quitSeafile();
    void onActivated(QSystemTrayIcon::ActivationReason);
    void prepareContextMenu();
    void toggleMainWindow();
    void onClick();
    void rotateTrayIcon();
    void refreshTrayIcon();
    void openHelp();
    void openLogDirectory();
    void about();
    void onSeahubNotificationsChanged();
    void viewUnreadNotifications();

private:
    Q_DISABLE_COPY(SeafileTrayIcon)

    void createActions();
    void createContextMenu();
    bool allServersConnected();

    QIcon stateToIcon(TrayState state);
    QIcon getIcon(const QString& name);

    QMenu *context_menu_;
    QMenu *help_menu_;

    // Actions for tray icon menu
    QAction *enable_auto_sync_action_;
    QAction *disable_auto_sync_action_;
    QAction *quit_action_;
    QAction *toggle_main_window_action_;
    QAction *settings_action_;
    QAction *open_log_directory_action_;
    QAction *view_unread_seahub_notifications_action_;

    QAction *about_action_;
    QAction *open_help_action_;

#if defined(Q_OS_MAC)
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

