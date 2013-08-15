#include <QMessageBox>
#include <QAction>
#include <QMenu>
#include <QSystemTrayIcon>
#include <QIcon>

#include <assert.h>

#include "account-mgr.h"
#include "configurator.h"
#include "daemon-mgr.h"
#include "rpc/rpc-client.h"
#include "ui/main-window.h"
#include "message-listener.h"

#include "seafile-applet.h"

SeafileApplet *seafApplet;

SeafileApplet::SeafileApplet()
    : configurator_(new Configurator),
      account_mgr_(new AccountManager),
      main_win_(new MainWindow),
      daemon_mgr_(new DaemonManager),
      message_listener_(new MessageListener),
      rpc_client_(new RpcClient),
      autoSync_(false)
{
    systemTray = new QSystemTrayIcon(this);
    systemTray->setIcon(getIcon(TRAY_DAEMON_UP));
    systemTray->setToolTip("Seafile");
    QAction *openAction = new QAction(tr("&Open"), this);
    connect(openAction, SIGNAL(triggered()), this, SLOT(openAdmin()));
    QAction *disableAction = new QAction(tr("Disable auto sync"), this);
    connect(disableAction, SIGNAL(triggered()), this, SLOT(disableAutoSync()));
    QAction *enableAction = new QAction(tr("Enable auto sync"), this);
    connect(enableAction, SIGNAL(triggered()), this, SLOT(enableAutoSync()));
    QAction *restartAction = new QAction(tr("&Restart Seafile"), this);
    connect(restartAction, SIGNAL(triggered()), this, SLOT(restartSeafile()));
    QAction *quitAction = new QAction(tr("&Quit"), this);
    connect(quitAction, SIGNAL(triggered()), this, SLOT(quitSeafile()));

    disableAutoSyncAction_ = disableAction;
    enableAutoSyncAction_ = enableAction;

    QMenu *pContextMenu = new QMenu(NULL);
    pContextMenu->addAction(openAction);
    pContextMenu->addSeparator();
    pContextMenu->addAction(disableAction);
    pContextMenu->addAction(enableAction);
    pContextMenu->addSeparator();
    pContextMenu->addAction(restartAction);
    pContextMenu->addSeparator();
    pContextMenu->addAction(quitAction);
    systemTray->setContextMenu(pContextMenu);
    enableAction->setVisible(false);
}

QIcon SeafileApplet::getIcon(TrayState state)
{
#ifdef Q_WS_MAC
    switch (state) {
    case TRAY_DAEMON_UP:
        return QIcon(":/images/mac_daemon_up.png");
    case TRAY_DAEMON_DOWN:
        return QIcon(":/images/mac_daemon_down.png");
    case TRAY_DAEMON_AUTOSYNC_DISABLED:
        return QIcon(":/images/mac_auto_sync_disabled.png");
    case TRAY_TRANSFER_1:
        return QIcon(":/images/mac_transfer_1.png");
    case TRAY_TRANSFER_2:
        return QIcon(":/images/mac_transfer_2.png");
    case TRAY_TRANSFER_3:
        return QIcon(":/images/mac_transfer_3.png");
    case TRAY_TRANSFER_4:
        return QIcon(":/images/mac_transfer_4.png");
    default:
        assert(0);
    }
#else
    switch (state) {
    case TRAY_DAEMON_UP:
        return QIcon(":/images/daemon_up.png");
    case TRAY_DAEMON_DOWN:
        return QIcon(":/images/daemon_down.png");
    case TRAY_DAEMON_AUTOSYNC_DISABLED:
        return QIcon(":/images/seafile_auto_sync_disabled.png");
    case TRAY_TRANSFER_1:
        return QIcon(":/images/seafile_transfer_1.png");
    case TRAY_TRANSFER_2:
        return QIcon(":/images/seafile_transfer_2.png");
    case TRAY_TRANSFER_3:
        return QIcon(":/images/seafile_transfer_3.png");
    case TRAY_TRANSFER_4:
        return QIcon(":/images/seafile_transfer_4.png");
    default:
        assert(0);
    }
#endif
    assert(0);
}

void SeafileApplet::setTrayState(TrayState state)
{
    systemTray->setIcon(getIcon(state));
}

void SeafileApplet::start()
{
    configurator_->checkInit();

    systemTray->show();

    daemon_mgr_->startCcnetDaemon();

    connect(daemon_mgr_, SIGNAL(ccnetDaemonConnected()),
            this, SLOT(onCcnetDaemonConnected()));
}

void SeafileApplet::onCcnetDaemonConnected()
{
    rpc_client_->connectDaemon();
    message_listener_->connectDaemon();
    main_win_->show();

    setTrayState (TRAY_DAEMON_UP);
    rpc_client_->reconnect();
}

void SeafileApplet::onCcnetDaemonDisconnected()
{
    qDebug("disconnected from ccnet daemon\n");
    setTrayState (TRAY_DAEMON_DOWN);
}

void SeafileApplet::openAdmin()
{
    main_win_->show();
}

void SeafileApplet::disableAutoSync()
{
    autoSync_ = false;
    disableAutoSyncAction_->setVisible(false);
    enableAutoSyncAction_->setVisible(true);
}

void SeafileApplet::enableAutoSync()
{
    autoSync_ = true;
    disableAutoSyncAction_->setVisible(true);
    enableAutoSyncAction_->setVisible(false);
}

void SeafileApplet::restartSeafile()
{
    qDebug("Restart\n");
}

void SeafileApplet::quitSeafile()
{
    daemon_mgr_->stopAll();    
    this->exit(0);
}

void SeafileApplet::exit(int code)
{
    // pay attention to use the global namespace, or it would cause stack
    // overflow
    ::exit(code);
}

void SeafileApplet::errorAndExit(const QString& error)
{
    QMessageBox::warning(NULL, tr("Seafile"), error, QMessageBox::Ok);
    exit(1);
}
