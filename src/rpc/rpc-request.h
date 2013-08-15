#ifndef SEAFILE_CLIENT_RPC_REQUEST_H
#define SEAFILE_CLIENT_RPC_REQUEST_H

#include <QObject>

extern "C" {

#include <searpc-client.h>

}

struct _GError;

/**
 * The base class for an async rpc request
 * Subclasses should implement:
 *
 * - request-specific constructor
 * - send()
 * - onRpcFinished(void *result)
 * - destructor
 *
 * And emit two kind of signals
 *
 * - success(T) T is the return type of the request
 * - failed() It is automatically emitted by the base class if GError is not NULL
 *
 */
class SeafileRpcRequest : public QObject {
    Q_OBJECT

public:
    virtual ~SeafileRpcRequest();

    virtual void send() = 0;

    static void callbackWrapper(void *result, void *data, _GError *error);

signals:
    void failed();

protected:
    SeafileRpcRequest();

    virtual void onRpcFinished(void *result) = 0;

    SearpcClient *getCcnetRpcClient();
    SearpcClient *getSeafRpcClient();

private:
    Q_DISABLE_COPY(SeafileRpcRequest)

    void emitFailed();
};

#endif // SEAFILE_CLIENT_RPC_REQUEST_H
