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

#include "seafile-applet.h"
#include "rpc/rpc-client.h"
#include "main-window.h"
#include "settings-dialog.h"
#include "settings-mgr.h"
#include "seahub-notifications-monitor.h"

#include "tray-icon.h"
#if defined(Q_OS_MAC)
#include "traynotificationmanager.h"
#endif

#if defined(Q_OS_LINUX)
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCall>
#endif

namespace {

const int kRefreshInterval = 1000;
const int kRotateTrayIconIntervalMilli = 250;

#if defined(Q_OS_WIN)
#include <windows.h>
bool
isWindowsVistaOrHigher()
{
    OSVERSIONINFOEX ver = {0};

    ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

    if (!GetVersionEx((LPOSVERSIONINFO)&ver)) {
        return false;
    }

    if (ver.dwMajorVersion >= 6) {
        return true;
    }

    return false;
}
#endif

}

SeafileTrayIcon::SeafileTrayIcon(QObject *parent)
    : QSystemTrayIcon(parent),
      nth_trayicon_(0),
      rotate_counter_(0)
{
    setState(STATE_DAEMON_DOWN);
    rotate_timer_ = new QTimer(this);
    connect(rotate_timer_, SIGNAL(timeout()), this, SLOT(rotateTrayIcon()));

    refresh_timer_ = new QTimer(this);
    connect(refresh_timer_, SIGNAL(timeout()), this, SLOT(refreshTrayIcon()));

    createActions();
    createContextMenu();

    connect(this, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
            this, SLOT(onActivated(QSystemTrayIcon::ActivationReason)));

    connect(SeahubNotificationsMonitor::instance(), SIGNAL(notificationsChanged()),
            this, SLOT(onSeahubNotificationsChanged()));

    hide();

#if defined(Q_OS_MAC)
    tnm = new TrayNotificationManager(this);
#endif
}

void SeafileTrayIcon::start()
{
    show();
    refresh_timer_->start(kRefreshInterval);
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

    toggle_main_window_action_ = new QAction(tr("Show main window"), this);
    connect(toggle_main_window_action_, SIGNAL(triggered()), this, SLOT(toggleMainWindow()));

    settings_action_ = new QAction(tr("Settings"), this);
    connect(settings_action_, SIGNAL(triggered()), this, SLOT(showSettingsWindow()));

    about_action_ = new QAction(tr("&About"), this);
    about_action_->setStatusTip(tr("Show the application's About box"));
    connect(about_action_, SIGNAL(triggered()), this, SLOT(about()));

    open_help_action_ = new QAction(tr("&Online help"), this);
    open_help_action_->setStatusTip(tr("open seafile online help"));
    connect(open_help_action_, SIGNAL(triggered()), this, SLOT(openHelp()));
}

void SeafileTrayIcon::createContextMenu()
{
    help_menu_ = new QMenu(tr("Help"), NULL);
    help_menu_->addAction(about_action_);
    help_menu_->addAction(open_help_action_);

    context_menu_ = new QMenu(NULL);
    context_menu_->addAction(view_unread_seahub_notifications_action_);
    context_menu_->addAction(toggle_main_window_action_);
    context_menu_->addAction(settings_action_);
    context_menu_->addMenu(help_menu_);
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

    if (!seafApplet->mainWindow()->isVisible()) {
        toggle_main_window_action_->setText(tr("Show main window"));
    } else {
        toggle_main_window_action_->setText(tr("Hide main window"));
    }

    view_unread_seahub_notifications_action_->setVisible(state_ == STATE_HAVE_UNREAD_MESSAGE);
}

void SeafileTrayIcon::notify(const QString &title, const QString &content)
{
#if defined(Q_OS_MAC)
    QIcon icon(":/images/info.png");
    TrayNotificationWidget* trayNotification = new TrayNotificationWidget(icon.pixmap(32, 32), title, content);
    tnm->append(trayNotification);
#else
    this->showMessage(title, content);
#endif
}

void SeafileTrayIcon::rotate(bool start)
{
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

void SeafileTrayIcon::showMessage(const QString & title, const QString & message, MessageIcon icon, int millisecondsTimeoutHint)
{
#if defined(Q_OS_LINUX)
    QList<QVariant> args = QList<QVariant>() << "seafile" << quint32(0) << "dialog-information"
                                             << title << message << QStringList () << QVariantMap() << qint32(-1);
    QDBusMessage method = QDBusMessage::createMethodCall("org.freedesktop.Notifications","/org/freedesktop/Notifications", "", "Notify");
    method.setArguments(args);
    QDBusConnection::sessionBus().asyncCall(method);
#else
    QSystemTrayIcon::showMessage(title, message, icon, millisecondsTimeoutHint);
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

    // the following two lines solving the problem of tray icon
    // disappear in ubuntu 13.04
#if defined(Q_OS_LINUX)
    hide();
    show();
#endif

    setToolTip(tool_tip);
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
#if defined(Q_OS_WIN)
    QString prefix = ":/images/win/";

    switch (state) {
    case STATE_DAEMON_UP:
        return getIcon(prefix + "daemon_up.ico");
    case STATE_DAEMON_DOWN:
        return getIcon(prefix + "daemon_down.ico");
    case STATE_DAEMON_AUTOSYNC_DISABLED:
        return getIcon(prefix + "seafile_auto_sync_disabled.ico");
    case STATE_TRANSFER_1:
        return getIcon(prefix + "seafile_transfer_1.ico");
    case STATE_TRANSFER_2:
        return getIcon(prefix + "seafile_transfer_2.ico");
    case STATE_SERVERS_NOT_CONNECTED:
        return getIcon(prefix + "seafile_warning.ico");
    case STATE_HAVE_UNREAD_MESSAGE:
        return getIcon(prefix + "notification.ico");
    }
#elif defined(Q_OS_MAC)
    switch (state) {
    case STATE_DAEMON_UP:
        return getIcon(":/images/mac/daemon_up.png");
    case STATE_DAEMON_DOWN:
        return getIcon(":/images/mac/daemon_down.png");
    case STATE_DAEMON_AUTOSYNC_DISABLED:
        return getIcon(":/images/mac/seafile_auto_sync_disabled.png");
    case STATE_TRANSFER_1:
        return getIcon(":/images/mac/seafile_transfer_1.png");
    case STATE_TRANSFER_2:
        return getIcon(":/images/mac/seafile_transfer_2.png");
    case STATE_SERVERS_NOT_CONNECTED:
        return getIcon(":/images/mac/seafile_warning.png");
    case STATE_HAVE_UNREAD_MESSAGE:
        return getIcon(":/images/mac/notification.png");
    }
#else
    switch (state) {
    case STATE_DAEMON_UP:
        return getIcon(":/images/daemon_up.png");
    case STATE_DAEMON_DOWN:
        return getIcon(":/images/daemon_down.png");
    case STATE_DAEMON_AUTOSYNC_DISABLED:
        return getIcon(":/images/seafile_auto_sync_disabled.png");
    case STATE_TRANSFER_1:
        return getIcon(":/images/seafile_transfer_1.png");
    case STATE_TRANSFER_2:
        return getIcon(":/images/seafile_transfer_2.png");
    case STATE_SERVERS_NOT_CONNECTED:
        return getIcon(":/images/seafile_warning.png");
    case STATE_HAVE_UNREAD_MESSAGE:
        return getIcon(":/images/notification.png");
    }
#endif
}

void SeafileTrayIcon::toggleMainWindow()
{
    MainWindow *main_win = seafApplet->mainWindow();
    if (!main_win->isVisible()) {
        main_win->showWindow();
    } else {
        main_win->hide();
    }
}

void SeafileTrayIcon::about()
{
    QMessageBox::about(seafApplet->mainWindow(), tr("About %1").arg(getBrand()),
                       tr("<h2>%1 Client %2</h2>").arg(getBrand()).arg(
                           STRINGIZE(SEAFILE_CLIENT_VERSION)));
}

void SeafileTrayIcon::openHelp()
{
    QString url;
    if (QLocale::system().name() == "zh_CN") {
        url = "http://seafile.com/help/install_v2/";
    } else {
        url = "http://seafile.com/en/help/install_v2/";
    }

    QDesktopServices::openUrl(QUrl(url));
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
        onClick();
        break;
    default:
        return;
    }
#endif
}

void SeafileTrayIcon::onClick()
{
    if (state_ == STATE_HAVE_UNREAD_MESSAGE) {
        viewUnreadNotifications();
    } else {
        toggleMainWindow();
    }
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
    seafApplet->exit(0);
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

    bool all_server_connected = allServersConnected();
    if (!all_server_connected) {
        setState(STATE_SERVERS_NOT_CONNECTED, tr("some servers not connected"));
        return;
    }

    setState(STATE_DAEMON_UP);
}

bool SeafileTrayIcon::allServersConnected()
{
    if (!seafApplet->started()) {
        return true;
    }

    GList *servers = NULL;
    if (seafApplet->rpcClient()->getServers(&servers) < 0) {
        return true;
    }

    if (!servers) {
        return true;
    }

    GList *ptr;
    bool all_server_connected = true;
    for (ptr = servers; ptr ; ptr = ptr->next) {
        CcnetPeer *server = (CcnetPeer *)ptr->data;
        if (server->net_state != PEER_CONNECTED) {
            all_server_connected = false;
            break;
        }
    }

    g_list_foreach (servers, (GFunc)g_object_unref, NULL);
    g_list_free (servers);

    return all_server_connected;
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
