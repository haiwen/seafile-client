#ifndef SEAFILE_CLIENT_TRAY_ICON_H
#define SEAFILE_CLIENT_TRAY_ICON_H

#include <QSystemTrayIcon>

class QAction;
class QMenu;

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
    };

    void setState(TrayState);
    void notify(const QString &title, const QString &content);
    void rotate(bool start);

private slots:
    void disableAutoSync();
    void enableAutoSync();
    void quitSeafile();
    void onActivated(QSystemTrayIcon::ActivationReason);
    void prepareContextMenu();
    void toggleMainWindow();
    void rotateTrayIcon();

private:
    Q_DISABLE_COPY(SeafileTrayIcon)

    void createActions();
    void createContextMenu();

    QIcon stateToIcon(TrayState state);
    void resetToolTip();

    QMenu *context_menu_;
    // Actions for tray icon menu
    QAction *enable_auto_sync_action_;
    QAction *disable_auto_sync_action_;
    QAction *quit_action_;
    QAction *toggle_main_window_action_;

    QTimer *rotate_timer_;
    int nth_trayicon_;
    int rotate_counter_;
    bool auto_sync_;
};

#endif // SEAFILE_CLIENT_TRAY_ICON_H

