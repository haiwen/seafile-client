#ifndef SEAFILE_CLIENT_TRAY_ICON_H
#define SEAFILE_CLIENT_TRAY_ICON_H

#include <QSystemTrayIcon>
#include <QHash>
#include <QQueue>

#include "account.h"
#include "log-uploader.h"

class ApiError;
class QAction;
class QMenu;
class QMenuBar;
#if defined(Q_OS_MAC)
class TrayNotificationManager;
#endif
class AboutDialog;
class SyncErrorsDialog;
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

    TrayState state() const { return state_; }
    void setState(TrayState state, const QString& tip=QString());
    void rotate(bool start);

    void reloadTrayIcon();

    void showMessage(const QString& title,
                     const QString& message,
                     const QString& repo_id = QString(),
                     const QString& commit_id = QString(),
                     const QString& previous_commit_id = QString(),
                     MessageIcon icon = Information,
                     int millisecondsTimeoutHint = 10000);

public slots:
    void showSettingsWindow();

private slots:
    void disableAutoSync();
    void enableAutoSync();
    void quitSeafile();
    void onActivated(QSystemTrayIcon::ActivationReason);
    void prepareContextMenu();
    void showMainWindow();
    void rotateTrayIcon();
    void refreshTrayIcon();
    void refreshTrayIconToolTip();
    void openHelp();
    void openSeafileFolder();
    void openLogDirectory();
    void uploadLogDirectory();
    void about();
    void checkTrayIconMessageQueue();
    void clearUploader();

    // only used on windows
    void onMessageClicked();

    void showSyncErrorsDialog();

private:
    Q_DISABLE_COPY(SeafileTrayIcon)

    void createActions();
    void createContextMenu();
    void createGlobalMenuBar();

    QIcon stateToIcon(TrayState state);
    QIcon getIcon(const QString& name);

    QMenu *context_menu_;
    QMenu *help_menu_;
    QMenu *global_menu_;
    QMenu *dock_menu_;
    QMenuBar *global_menubar_;

    // Actions for tray icon menu
    QAction *enable_auto_sync_action_;
    QAction *disable_auto_sync_action_;
    QAction *quit_action_;
    QAction *show_main_window_action_;
    QAction *settings_action_;
    QAction *open_seafile_folder_action_;
    QAction *open_log_directory_action_;
    QAction *upload_log_directory_action_;
    QAction *show_sync_errors_action_;

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

    QString repo_id_;
    QString commit_id_;
    QString previous_commit_id_;

    QHash<QString, QIcon> icon_cache_;

    struct TrayMessage {
        QString title;
        QString message;
        MessageIcon icon;
        QString repo_id;
        QString commit_id;
        QString previous_commit_id;
    };

    // Use a queue to gurantee each tray notification message would be
    // displayed at least several seconds.
    QQueue<TrayMessage> pending_messages_;
    qint64 next_message_msec_;

    SyncErrorsDialog *sync_errors_dialog_;
    AboutDialog *about_dialog_;
    LogDirUploader *log_dir_uploader_;
};

#endif // SEAFILE_CLIENT_TRAY_ICON_H
