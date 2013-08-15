#include <QMessageBox>

#include "account-mgr.h"
#include "configurator.h"
#include "daemon-mgr.h"
#include "message-listener.h"
#include "settings-mgr.h"
#include "rpc/rpc-client.h"
#include "ui/main-window.h"
#include "ui/tray-icon.h"

#include "seafile-applet.h"

SeafileApplet *seafApplet;

SeafileApplet::SeafileApplet()
    : configurator_(new Configurator),
      account_mgr_(new AccountManager),
      main_win_(new MainWindow),
      daemon_mgr_(new DaemonManager),
      message_listener_(new MessageListener),
      rpc_client_(new SeafileRpcClient),
      settings_mgr_(new SettingsManager),
      in_exit_(false)
{
    tray_icon_ = new SeafileTrayIcon(this);
}

void SeafileApplet::start()
{
    tray_icon_->show();
    configurator_->checkInit();

    daemon_mgr_->startCcnetDaemon();

    connect(daemon_mgr_, SIGNAL(daemonStarted()),
            this, SLOT(onDaemonStarted()));
}

void SeafileApplet::onDaemonStarted()
{
    // tray_icon_->showMessage("Seafile", "daemon is started");
    main_win_->show();
    tray_icon_->setState(SeafileTrayIcon::STATE_DAEMON_UP);

    rpc_client_->connectDaemon();
    message_listener_->connectDaemon();
}

void SeafileApplet::exit(int code)
{
    // Must use the global namespace, or the "exit" would call itself util
    // stack overflow
    daemon_mgr_->stopAll();
    ::exit(code);
}

void SeafileApplet::errorAndExit(const QString& error)
{
    in_exit_ = true;
    QMessageBox::warning(main_win_, tr("Seafile"), error, QMessageBox::Ok);
    this->exit(1);
}
