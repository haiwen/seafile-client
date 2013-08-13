#include <QMessageBox>

#include "account-mgr.h"
#include "rpc/rpc-client.h"
#include "ui/main-window.h"
#include "configurator.h"

#include "seafile-applet.h"

SeafileApplet *seafApplet;

SeafileApplet::SeafileApplet()
    : configurator(new Configurator),
      account_mgr(new AccountManager),
      main_win(new MainWindow),
      rpc_client(0)
{
}

void SeafileApplet::start()
{
    configurator->checkInit();

    rpc_client = new RpcClient(configurator->ccnetDir());
    rpc_client->start();
}

void SeafileApplet::exit(int code)
{
    exit(code);
}

void SeafileApplet::errorAndExit(const QString& error)
{
    QMessageBox::warning(NULL, tr("Seafile"), error, QMessageBox::Ok);
    exit(1);
}
