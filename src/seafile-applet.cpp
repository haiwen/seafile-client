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
}

void SeafileApplet::onCcnetDaemonConnected()
{
    rpc_client_->connectDaemon();
    message_listener_->connectDaemon();
    main_win_->show();
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
