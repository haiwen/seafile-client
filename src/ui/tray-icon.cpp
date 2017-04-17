extern "C" {

#include <ccnet/peer.h>

}
#include <QtGlobal>

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
#include <QtWidgets>
#else
#include <QtGui>
#endif
#include <QApplication>
#include <QDesktopServices>
#include <QSet>
#include <QDebug>
#include <QMenuBar>
#include <QRunnable>
#include <QSysInfo>

#include "rpc/local-repo.h"
#include "utils/utils.h"
#include "utils/utils-mac.h"
#include "utils/file-utils.h"
#include "seafile-applet.h"
#include "configurator.h"
#include "rpc/rpc-client.h"
#include "main-window.h"
#include "settings-dialog.h"
#include "settings-mgr.h"
#include "seahub-notifications-monitor.h"
#include "server-status-service.h"
#include "api/commit-details.h"
#include "sync-errors-dialog.h"

#include "tray-icon.h"
#if defined(Q_OS_MAC)
#include "traynotificationmanager.h"
// QT's platform apis
// http://qt-project.org/doc/qt-4.8/exportedfunctions.html
extern void qt_mac_set_dock_menu(QMenu *menu);
#endif
#include "utils/utils-mac.h"

#if defined(Q_OS_LINUX)
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCall>
#endif

#include "src/ui/about-dialog.h"

namespace {

const int kRefreshInterval = 1000;
const int kRotateTrayIconIntervalMilli = 250;
const int kMessageDisplayTimeMSecs = 5000;

#ifdef Q_OS_MAC
void darkmodeWatcher(bool /*new Value*/) {
    seafApplet->trayIcon()->reloadTrayIcon();
}
#endif

QString folderToShow(const CommitDetails& details, const QString& worktree)
{
    QString path = worktree;
    if (!details.added_files.empty()) {
        path = pathJoin(worktree, details.added_files[0]);
    } else if (!details.modified_files.empty()) {
        path = pathJoin(worktree, details.modified_files[0]);
    } else if (!details.added_dirs.empty()) {
        path = pathJoin(worktree, details.added_dirs[0]);
    } else if (!details.renamed_files.empty()) {
        path = pathJoin(worktree, details.renamed_files[0].second);
    } else if (!details.deleted_files.empty()) {
        path = getParentPath(pathJoin(worktree, details.deleted_files[0]));
    } else if (!details.deleted_dirs.empty()) {
        path = getParentPath(pathJoin(worktree, details.deleted_dirs[0]));
    }
    return path;
}

// Read the commit diff from daemon rpc. We need to run it in a separate thread
// since this may block for a while.
class DiffReader : public QRunnable
{
public:
    DiffReader(const LocalRepo &repo,
               const QString &previous_commit_id,
               const QString &commit_id)
        : repo_(repo),
          commit_id_(commit_id),
          previous_commit_id_(previous_commit_id){};

    void run()
    {
        CommitDetails details;
        seafApplet->rpcClient()->getCommitDiff(
            repo_.id, previous_commit_id_, commit_id_, &details);
        showInGraphicalShell(folderToShow(details, repo_.worktree));
    }

private:
    const LocalRepo repo_;
    const QString commit_id_;
    const QString previous_commit_id_;
};


} // namespace

SeafileTrayIcon::SeafileTrayIcon(QObject *parent)
    : QSystemTrayIcon(parent),
      nth_trayicon_(0),
      rotate_counter_(0),
      state_(STATE_DAEMON_UP),
      next_message_msec_(0),
      sync_errors_dialog_(nullptr),
      about_dialog_(nullptr)
{
    setState(STATE_DAEMON_DOWN);
    rotate_timer_ = new QTimer(this);
    connect(rotate_timer_, SIGNAL(timeout()), this, SLOT(rotateTrayIcon()));

    refresh_timer_ = new QTimer(this);
    connect(refresh_timer_, SIGNAL(timeout()), this, SLOT(refreshTrayIcon()));
    connect(refresh_timer_, SIGNAL(timeout()), this, SLOT(refreshTrayIconToolTip()));
#if defined(Q_OS_WIN32)
    connect(refresh_timer_, SIGNAL(timeout()), this, SLOT(checkTrayIconMessageQueue()));
#endif

    createActions();
    createContextMenu();

    connect(this, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
            this, SLOT(onActivated(QSystemTrayIcon::ActivationReason)));

    connect(SeahubNotificationsMonitor::instance(), SIGNAL(notificationsChanged()),
            this, SLOT(onSeahubNotificationsChanged()));

#if !defined(Q_OS_LINUX)
    connect(this, SIGNAL(messageClicked()),
            this, SLOT(onMessageClicked()));
#endif

    hide();

    createGlobalMenuBar();
#if defined(Q_OS_MAC)
    tnm = new TrayNotificationManager(this);
#endif
}

void SeafileTrayIcon::start()
{
    show();
    refresh_timer_->start(kRefreshInterval);
#if defined(Q_OS_MAC)
    utils::mac::set_darkmode_watcher(&darkmodeWatcher);
#endif
}

void SeafileTrayIcon::createActions()
{
    disable_auto_sync_action_ = new QAction(tr("Disable auto sync"), this);
    connect(disable_auto_sync_action_, SIGNAL(triggered()), this, SLOT(disableAutoSync()));

    enable_auto_sync_action_ = new QAction(tr("Enable auto sync"), this);
    connect(enable_auto_sync_action_, SIGNAL(triggered()), this, SLOT(enableAutoSync()));

    view_unread_seahub_notifications_action_ = new QAction(tr("View unread notifications"), this);
    connect(view_unread_seahub_notifications_action_, SIGNAL(triggered()),
            this, SLOT(viewUnreadNotifications()));

    quit_action_ = new QAction(tr("&Quit"), this);
    connect(quit_action_, SIGNAL(triggered()), this, SLOT(quitSeafile()));

    show_main_window_action_ = new QAction(tr("Show main window"), this);
    connect(show_main_window_action_, SIGNAL(triggered()), this, SLOT(showMainWindow()));

    settings_action_ = new QAction(tr("Settings"), this);
    connect(settings_action_, SIGNAL(triggered()), this, SLOT(showSettingsWindow()));

    open_seafile_folder_action_ = new QAction(tr("Open %1 &folder").arg(getBrand()), this);
    open_seafile_folder_action_->setStatusTip(tr("open %1 folder").arg(getBrand()));
    connect(open_seafile_folder_action_, SIGNAL(triggered()), this, SLOT(openSeafileFolder()));

    open_log_directory_action_ = new QAction(tr("Open &logs folder"), this);
    open_log_directory_action_->setStatusTip(tr("open %1 log folder").arg(getBrand()));
    connect(open_log_directory_action_, SIGNAL(triggered()), this, SLOT(openLogDirectory()));

    show_sync_errors_action_ = new QAction(tr("Show file sync errors"), this);
    show_sync_errors_action_->setStatusTip(tr("Show file sync errors"));
    connect(show_sync_errors_action_, SIGNAL(triggered()), this, SLOT(showSyncErrorsDialog()));

    about_action_ = new QAction(tr("&About"), this);
    about_action_->setStatusTip(tr("Show the application's About box"));
    connect(about_action_, SIGNAL(triggered()), this, SLOT(about()));

    open_help_action_ = new QAction(tr("&Online help"), this);
    open_help_action_->setStatusTip(tr("open %1 online help").arg(getBrand()));
    connect(open_help_action_, SIGNAL(triggered()), this, SLOT(openHelp()));
}

void SeafileTrayIcon::createContextMenu()
{
    // help_menu_ = new QMenu(tr("Help"), NULL);
    // help_menu_->addAction(about_action_);
    // help_menu_->addAction(open_help_action_);

    context_menu_ = new QMenu(NULL);
    context_menu_->addAction(view_unread_seahub_notifications_action_);
    context_menu_->addAction(show_main_window_action_);
    context_menu_->addAction(open_seafile_folder_action_);
    context_menu_->addAction(settings_action_);
    context_menu_->addAction(open_log_directory_action_);
    context_menu_->addAction(show_sync_errors_action_);
    // context_menu_->addMenu(help_menu_);
    context_menu_->addSeparator();
    context_menu_->addAction(about_action_);
    context_menu_->addAction(open_help_action_);
    context_menu_->addSeparator();
    context_menu_->addAction(enable_auto_sync_action_);
    context_menu_->addAction(disable_auto_sync_action_);
    context_menu_->addSeparator();
    context_menu_->addAction(quit_action_);

    setContextMenu(context_menu_);
    connect(context_menu_, SIGNAL(aboutToShow()), this, SLOT(prepareContextMenu()));
}

void SeafileTrayIcon::prepareContextMenu()
{
    if (seafApplet->settingsManager()->autoSync()) {
        enable_auto_sync_action_->setVisible(false);
        disable_auto_sync_action_->setVisible(true);
    } else {
        enable_auto_sync_action_->setVisible(true);
        disable_auto_sync_action_->setVisible(false);
    }

    view_unread_seahub_notifications_action_->setVisible(state_ == STATE_HAVE_UNREAD_MESSAGE);
}

void SeafileTrayIcon::createGlobalMenuBar()
{
    // support it only on mac os x currently
    // TODO: destroy the objects when seafile closes
#ifdef Q_OS_MAC
    // create qmenu used in menubar and docker menu
    global_menu_ = new QMenu(tr("File"));
    global_menu_->addAction(view_unread_seahub_notifications_action_);
    global_menu_->addAction(show_main_window_action_);
    global_menu_->addAction(open_seafile_folder_action_);
    global_menu_->addAction(settings_action_);
    global_menu_->addAction(open_log_directory_action_);
    global_menu_->addAction(show_sync_errors_action_);
    global_menu_->addSeparator();
    global_menu_->addAction(enable_auto_sync_action_);
    global_menu_->addAction(disable_auto_sync_action_);

    global_menubar_ = new QMenuBar(0);
    global_menubar_->addMenu(global_menu_);
    // TODO fix the line below which crashes under qt5.4.0
    //global_menubar_->addMenu(help_menu_);
    global_menubar_->setNativeMenuBar(true);
    qApp->setAttribute(Qt::AA_DontUseNativeMenuBar, false);

#if (QT_VERSION >= QT_VERSION_CHECK(5, 2, 0))
    global_menu_->setAsDockMenu(); // available after qt5.2.0
#else
    qt_mac_set_dock_menu(global_menu_); // deprecated in latest qt
#endif
    // create QMenuBar that has no parent, so we can share the global menubar
#endif // Q_OS_MAC
}

void SeafileTrayIcon::rotate(bool start)
{
    /* tray icon should not be refreshed on Gnome according to their guidelines */
#if defined(Q_OS_LINUX)
    const char *env = g_getenv("DESKTOP_SESSION");
    if (env != NULL &&
        (strcmp(env, "gnome") == 0 || strcmp(env, "gnome-wayland") == 0)) {
        return;
    }
#endif

    if (start) {
        rotate_counter_ = 0;
        if (!rotate_timer_->isActive()) {
            nth_trayicon_ = 0;
            rotate_timer_->start(kRotateTrayIconIntervalMilli);
        }
    } else {
        rotate_timer_->stop();
    }
}

void SeafileTrayIcon::showMessage(const QString &title,
                                  const QString &message,
                                  const QString &repo_id,
                                  const QString &commit_id,
                                  const QString &previous_commit_id,
                                  MessageIcon icon,
                                  int millisecondsTimeoutHint)
{
#ifdef Q_OS_MAC
    repo_id_ = repo_id;
    commit_id_ = commit_id;
    previous_commit_id_ = previous_commit_id;
    if (QSysInfo::MacintoshVersion < QSysInfo::MV_MOUNTAINLION) {
        // qWarning("using old style notifications");
        QIcon info_icon(":/images/info.png");
        TrayNotificationWidget* trayNotification = new TrayNotificationWidget(info_icon.pixmap(32, 32), title, message);
        tnm->append(trayNotification);
        return;
    }
    // qWarning("using new style notifications");

    QSystemTrayIcon::showMessage(title, message, icon, millisecondsTimeoutHint);
#elif defined(Q_OS_LINUX)
    repo_id_ = repo_id;
    Q_UNUSED(icon);
    QVariantMap hints;
    QString brand = getBrand();
    hints["resident"] = QVariant(true);
    hints["desktop-entry"] = QVariant(brand);
    QList<QVariant> args = QList<QVariant>() << brand << quint32(0) << brand
                                             << title << message << QStringList () << hints << qint32(-1);
    QDBusMessage method = QDBusMessage::createMethodCall("org.freedesktop.Notifications","/org/freedesktop/Notifications", "org.freedesktop.Notifications", "Notify");
    method.setArguments(args);
    QDBusConnection::sessionBus().asyncCall(method);
#else
    TrayMessage msg;
    msg.title = title;
    msg.message = message;
    msg.icon = icon;
    msg.repo_id = repo_id;
    msg.commit_id = commit_id;
    msg.previous_commit_id = previous_commit_id;
    pending_messages_.enqueue(msg);
#endif
}

void SeafileTrayIcon::rotateTrayIcon()
{
    if (rotate_counter_ >= 8 || !seafApplet->settingsManager()->autoSync()) {
        rotate_timer_->stop();
        if (!seafApplet->settingsManager()->autoSync())
            setState (STATE_DAEMON_AUTOSYNC_DISABLED);
        else
            setState (STATE_DAEMON_UP);
        return;
    }

    TrayState states[] = { STATE_TRANSFER_1, STATE_TRANSFER_2 };
    int index = nth_trayicon_ % 2;
    setIcon(stateToIcon(states[index]));

    nth_trayicon_++;
    rotate_counter_++;
}

void SeafileTrayIcon::setState(TrayState state, const QString& tip)
{
    if (state_ == state) {
        return;
    }

    QString tool_tip = tip.isEmpty() ? getBrand() : tip;

    setIcon(stateToIcon(state));
    setToolTip(tool_tip);
}

void SeafileTrayIcon::reloadTrayIcon()
{
    setIcon(stateToIcon(state_));

#if defined(Q_OS_LINUX)
    hide();
    show();
#endif
}

QIcon SeafileTrayIcon::getIcon(const QString& name)
{
    if (icon_cache_.contains(name)) {
        return icon_cache_[name];
    }

    QIcon icon(name);
    icon_cache_[name] = icon;
    return icon;
}

QIcon SeafileTrayIcon::stateToIcon(TrayState state)
{
    state_ = state;
#if defined(Q_OS_WIN32)
    QString icon_name;
    switch (state) {
    case STATE_DAEMON_UP:
        icon_name = ":/images/win/daemon_up.ico";
        break;
    case STATE_DAEMON_DOWN:
        icon_name = ":/images/win/daemon_down.ico";
        break;
    case STATE_DAEMON_AUTOSYNC_DISABLED:
        icon_name = ":/images/win/seafile_auto_sync_disabled.ico";
        break;
    case STATE_TRANSFER_1:
        icon_name = ":/images/win/seafile_transfer_1.ico";
        break;
    case STATE_TRANSFER_2:
        icon_name = ":/images/win/seafile_transfer_2.ico";
        break;
    case STATE_SERVERS_NOT_CONNECTED:
        icon_name = ":/images/win/seafile_warning.ico";
        break;
    case STATE_HAVE_UNREAD_MESSAGE:
        icon_name = ":/images/win/notification.ico";
        break;
    }
    return getIcon(icon_name);
#elif defined(Q_OS_MAC)
    bool isDarkMode = utils::mac::is_darkmode();
    // filename = icon_name + ?white + .png
    QString icon_name;

    switch (state) {
    case STATE_DAEMON_UP:
        icon_name = ":/images/mac/daemon_up";
        break;
    case STATE_DAEMON_DOWN:
        icon_name = ":/images/mac/daemon_down.png";
        break;
    case STATE_DAEMON_AUTOSYNC_DISABLED:
        icon_name = ":/images/mac/seafile_auto_sync_disabled";
        break;
    case STATE_TRANSFER_1:
        icon_name = ":/images/mac/seafile_transfer_1";
        break;
    case STATE_TRANSFER_2:
        icon_name = ":/images/mac/seafile_transfer_2";
        break;
    case STATE_SERVERS_NOT_CONNECTED:
        icon_name = ":/images/mac/seafile_warning";
        break;
    case STATE_HAVE_UNREAD_MESSAGE:
        icon_name = ":/images/mac/notification";
        break;
    }
    return getIcon(icon_name + (isDarkMode ? "_white" : "") + ".png");
#else
    QString icon_name;
    switch (state) {
    case STATE_DAEMON_UP:
        icon_name = ":/images/daemon_up.png";
        break;
    case STATE_DAEMON_DOWN:
        icon_name = ":/images/daemon_down.png";
        break;
    case STATE_DAEMON_AUTOSYNC_DISABLED:
        icon_name = ":/images/seafile_auto_sync_disabled.png";
        break;
    case STATE_TRANSFER_1:
        icon_name = ":/images/seafile_transfer_1.png";
        break;
    case STATE_TRANSFER_2:
        icon_name = ":/images/seafile_transfer_2.png";
        break;
    case STATE_SERVERS_NOT_CONNECTED:
        icon_name = ":/images/seafile_warning.png";
        break;
    case STATE_HAVE_UNREAD_MESSAGE:
        icon_name = ":/images/notification.png";
        break;
    }
    return getIcon(icon_name);
#endif
}

void SeafileTrayIcon::showMainWindow()
{
    MainWindow *main_win = seafApplet->mainWindow();
    main_win->showWindow();
}

void SeafileTrayIcon::about()
{
    if (!about_dialog_) {
        about_dialog_ = new AboutDialog();
    }
    about_dialog_->show();
    about_dialog_->raise();
    about_dialog_->activateWindow();
    return;

//     QMessageBox::about(seafApplet->mainWindow(), tr("About %1").arg(getBrand()),
//                        tr("<h2>%1 Client %2</h2>").arg(getBrand()).arg(
//                            STRINGIZE(SEAFILE_CLIENT_VERSION))
// #if defined(SEAFILE_CLIENT_REVISION)
//                        .append("<h4> REV %1 </h4>")
//                        .arg(STRINGIZE(SEAFILE_CLIENT_REVISION))
// #endif
//                        );
}

void SeafileTrayIcon::openHelp()
{
    QString url;
    if (QLocale::system().name() == "zh_CN") {
        url = "https://www.seafile.com/help/install/";
    } else {
        url = "https://www.seafile.com/en/help/install/";
    }

    QDesktopServices::openUrl(QUrl(url));
}

void SeafileTrayIcon::openSeafileFolder()
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(seafApplet->configurator()->seafileDir()).path()));
}

void SeafileTrayIcon::openLogDirectory()
{
    QString log_path = QDir(seafApplet->configurator()->ccnetDir()).absoluteFilePath("logs");
    QDesktopServices::openUrl(QUrl::fromLocalFile(log_path));
}

void SeafileTrayIcon::showSettingsWindow()
{
    seafApplet->settingsDialog()->show();
    seafApplet->settingsDialog()->raise();
    seafApplet->settingsDialog()->activateWindow();
}

void SeafileTrayIcon::onActivated(QSystemTrayIcon::ActivationReason reason)
{
#if !defined(Q_OS_MAC)
    switch(reason) {
    case QSystemTrayIcon::Trigger: // single click
    case QSystemTrayIcon::MiddleClick:
    case QSystemTrayIcon::DoubleClick:
        showMainWindow();
        break;
    default:
        return;
    }
#endif
}

void SeafileTrayIcon::disableAutoSync()
{
    seafApplet->settingsManager()->setAutoSync(false);
}

void SeafileTrayIcon::enableAutoSync()
{
    seafApplet->settingsManager()->setAutoSync(true);
}

void SeafileTrayIcon::quitSeafile()
{
    QCoreApplication::exit(0);
}

void SeafileTrayIcon::refreshTrayIcon()
{
    if (rotate_timer_->isActive()) {
        return;
    }

    int n_unread_msg = SeahubNotificationsMonitor::instance()->getUnreadNotifications();
    if (n_unread_msg > 0) {
        setState(STATE_HAVE_UNREAD_MESSAGE,
                 tr("You have %n message(s)", "", n_unread_msg));
        return;
    }

    if (!seafApplet->settingsManager()->autoSync()) {
        setState(STATE_DAEMON_AUTOSYNC_DISABLED,
                 tr("auto sync is disabled"));
        return;
    }

    if (!ServerStatusService::instance()->allServersConnected()) {
        setState(STATE_SERVERS_NOT_CONNECTED, tr("some servers not connected"));
        return;
    }

    setState(STATE_DAEMON_UP);
}

void SeafileTrayIcon::refreshTrayIconToolTip()
{
    if (!seafApplet->settingsManager()->autoSync())
        return;

    int up_rate, down_rate;
    if (seafApplet->rpcClient()->getUploadRate(&up_rate) < 0 ||
        seafApplet->rpcClient()->getDownloadRate(&down_rate) < 0) {
        return;
    }

    if (up_rate <= 0 && down_rate <= 0) {
        return;
    }

    QString uploadStr = tr("Uploading");
    QString downloadStr =  tr("Downloading");
    if (up_rate > 0 && down_rate > 0) {
        setToolTip(QString("%1 %2/s, %3 %4/s\n").
                   arg(uploadStr).arg(readableFileSize(up_rate)).
                   arg(downloadStr).arg(readableFileSize(down_rate)));
    } else if (up_rate > 0) {
        setToolTip(QString("%1 %2/s\n").
                   arg(uploadStr).arg(readableFileSize(up_rate)));
    } else /* down_rate > 0*/ {
        setToolTip(QString("%1 %2/s\n").
                   arg(downloadStr).arg(readableFileSize(down_rate)));
    }

    rotate(true);
}

void SeafileTrayIcon::onSeahubNotificationsChanged()
{
    if (!rotate_timer_->isActive()) {
        refreshTrayIcon();
    }
}

void SeafileTrayIcon::viewUnreadNotifications()
{
    SeahubNotificationsMonitor::instance()->openNotificationsPageInBrowser();
    refreshTrayIcon();
}


void SeafileTrayIcon::onMessageClicked()
{
    if (repo_id_.isEmpty())
        return;
    LocalRepo repo;
    if (seafApplet->rpcClient()->getLocalRepo(repo_id_, &repo) != 0 ||
        !repo.isValid() || repo.worktree_invalid)
        return;

    DiffReader *reader = new DiffReader(repo, previous_commit_id_, commit_id_);
    QThreadPool::globalInstance()->start(reader);
}

void SeafileTrayIcon::checkTrayIconMessageQueue()
{
    if (pending_messages_.empty()) {
        return;
    }

    qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (now < next_message_msec_) {
        return;
    }

    TrayMessage msg = pending_messages_.dequeue();
    QSystemTrayIcon::showMessage(msg.title, msg.message, msg.icon, kMessageDisplayTimeMSecs);
    repo_id_ = msg.repo_id;
    commit_id_ = msg.commit_id;
    next_message_msec_ = now + kMessageDisplayTimeMSecs;
}


void SeafileTrayIcon::showSyncErrorsDialog()
{
    if (sync_errors_dialog_ == nullptr) {
        sync_errors_dialog_ = new SyncErrorsDialog;
    }

    sync_errors_dialog_->updateErrors();
    sync_errors_dialog_->show();
    sync_errors_dialog_->raise();
    sync_errors_dialog_->activateWindow();
}
