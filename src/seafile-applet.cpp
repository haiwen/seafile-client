#include <QMessageBox>

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
      rpc_client_(new RpcClient)
{
}

void SeafileApplet::start()
{
    configurator_->checkInit();

    daemon_mgr_->startCcnetDaemon();

    connect(daemon_mgr_, SIGNAL(ccnetDaemonConnected()),
            this, SLOT(onCcnetDaemonConnected()));

    connect(daemon_mgr_, SIGNAL(ccnetDaemonDisconnected()), this,
            SLOT(onCcnetDaemonDisconnected()));
}

void SeafileApplet::onCcnetDaemonConnected()
{
    rpc_client_->reconnect();
    message_listener_->reconnect();
    main_win_->show();
}

void SeafileApplet::onCcnetDaemonDisconnected()
{
    qDebug("disconnected from ccnet daemon\n");
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
