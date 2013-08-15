#include <glib/gerror.h>

#include "seafile-applet.h"
#include "rpc-client.h"

#include "rpc-request.h"

SeafileRpcRequest::SeafileRpcRequest()
{
}

SeafileRpcRequest::~SeafileRpcRequest()
{
}

SearpcClient *SeafileRpcRequest::getCcnetRpcClient()
{
    return seafApplet->rpcClient()->ccnetRpcClient();
}

SearpcClient *SeafileRpcRequest::getSeafRpcClient()
{
    return seafApplet->rpcClient()->seafRpcClient();
}

void SeafileRpcRequest::emitFailed()
{
    emit failed();
}

void SeafileRpcRequest::callbackWrapper(void *result, void *data, GError *error)
{
    SeafileRpcRequest *req = static_cast<SeafileRpcRequest*>(data);
    if (error != NULL) {
        qDebug("failed to send rpc request: %s", error->message);
        req->emitFailed();
        return;
    }

    req->onRpcFinished(result);
}
