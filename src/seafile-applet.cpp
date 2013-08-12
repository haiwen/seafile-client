#include "seafile-applet.h"

#include "account-mgr.h"
#include "rpc/rpc-client.h"

SeafileApplet *seafApplet;

SeafileApplet::SeafileApplet()
{
    account_mgr = new AccountManager;
    rpc_client = new RpcClient;
}

void SeafileApplet::start()
{
    rpc_client->start();
}
