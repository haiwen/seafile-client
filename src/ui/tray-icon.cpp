#include <QtGui>
#include <QApplication>
#include <QSet>

#include "seafile-applet.h"
#include "main-window.h"
#include "settings-mgr.h"
#include "tray-icon.h"

SeafileTrayIcon::SeafileTrayIcon(QObject *parent)
    : QSystemTrayIcon(parent)
{
    setToolTip("Seafile");
    setState(STATE_DAEMON_DOWN);

    createActions();
    createContextMenu();

    connect(this, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
            this, SLOT(onActivated(QSystemTrayIcon::ActivationReason)));
}

// SeafileTrayIcon *SeafileTrayIcon::instance()
// {
//     /**
//      * This is a HACK around the qt trayicon bug on GNOME,
//      * See https://bugreports.qt-project.org/browse/QTBUG-30079
//      */
//     QSet<QWidget*> beforeWidgets = QApplication::topLevelWidgets().toSet();
//     SeafileTrayIcon *icon = new SeafileTrayIcon;
//     QSet<QWidget*> postWidgets = QApplication::topLevelWidgets().toSet();
//     postWidgets -= beforeWidgets;
//     if(!postWidgets.isEmpty())
//     {
//         QWidget* sysWidget = (*postWidgets.begin());
//         sysWidget->setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::SplashScreen );
//         sysWidget->show();
//     }

//     icon->init();

//     return icon;
// }

void SeafileTrayIcon::createActions()
{
    disable_auto_sync_action_ = new QAction(tr("Disable auto sync"), this);
    connect(disable_auto_sync_action_, SIGNAL(triggered()), this, SLOT(disableAutoSync()));

    enable_auto_sync_action_ = new QAction(tr("Enable auto sync"), this);
    connect(enable_auto_sync_action_, SIGNAL(triggered()), this, SLOT(enableAutoSync()));

    quit_action_ = new QAction(tr("&Quit"), this);
    connect(quit_action_, SIGNAL(triggered()), this, SLOT(quitSeafile()));

    toggle_main_window_action_ = new QAction(tr("Show main window"), this);
    connect(toggle_main_window_action_, SIGNAL(triggered()), this, SLOT(toggleMainWindow()));
}

void SeafileTrayIcon::createContextMenu()
{
    context_menu_ = new QMenu(NULL);

    context_menu_->addAction(toggle_main_window_action_);
    context_menu_->addSeparator();
    context_menu_->addAction(enable_auto_sync_action_);
    context_menu_->addAction(disable_auto_sync_action_);
    context_menu_->addSeparator();
    context_menu_->addAction(quit_action_);

    setContextMenu(context_menu_);

    connect(context_menu_, SIGNAL(aboutToShow()),
            this, SLOT(prepareContextMenu()));
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
}

void SeafileTrayIcon::setState(TrayState state)
{
    setIcon(stateToIcon(state));
}

QIcon SeafileTrayIcon::stateToIcon(TrayState state)
{
#ifdef Q_WS_MAC
    switch (state) {
    case STATE_DAEMON_UP:
        return QIcon(":/images/mac/daemon_up.png");
    case STATE_DAEMON_DOWN:
        return QIcon(":/images/mac/daemon_down.png");
    case STATE_DAEMON_AUTOSYNC_DISABLED:
        return QIcon(":/images/mac/seafile_auto_sync_disabled.png");
    case STATE_TRANSFER_1:
        return QIcon(":/images/mac/seafile_transfer_1.png");
    case STATE_TRANSFER_2:
        return QIcon(":/images/mac/seafile_transfer_2.png");
    }
#else
    switch (state) {
    case STATE_DAEMON_UP:
        return QIcon(":/images/daemon_up.png");
    case STATE_DAEMON_DOWN:
        return QIcon(":/images/daemon_down.png");
    case STATE_DAEMON_AUTOSYNC_DISABLED:
        return QIcon(":/images/seafile_auto_sync_disabled.png");
    case STATE_TRANSFER_1:
        return QIcon(":/images/seafile_transfer_1.png");
    case STATE_TRANSFER_2:
        return QIcon(":/images/seafile_transfer_2.png");
    }
#endif
}

void SeafileTrayIcon::toggleMainWindow()
{
    MainWindow *main_win = seafApplet->mainWindow();
    if (!main_win->isVisible()) {
        main_win->show();
        main_win->raise();
    } else {
        main_win->hide();
    }
}

void SeafileTrayIcon::onActivated(QSystemTrayIcon::ActivationReason reason)
{
    qDebug("onActivated: %d", reason);
    switch(reason) {
    case QSystemTrayIcon::Trigger: // single click
    case QSystemTrayIcon::MiddleClick:
    case QSystemTrayIcon::DoubleClick:
        toggleMainWindow();
        break;
    default:
        return;
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
