#ifndef SEAFILE_CLIENT_RPC_CLIENT_H
#define SEAFILE_CLIENT_RPC_CLIENT_H

#include <QObject>
#include <QString>

extern "C" {

struct _CcnetClient;
// Can't forward-declare type SearpcClient here because it is an anonymous typedef struct
#include <searpc-client.h>

}

class QTimer;

class RpcClient : QObject {
    Q_OBJECT

public:
    RpcClient(const QString& config_dir);
    void start();

private slots:
    void connectCcnetDaemon();

private:
    Q_DISABLE_COPY(RpcClient)

    QString config_dir_;
    _CcnetClient *sync_client_;
    SearpcClient *seafile_rpc_client_;

    QTimer *conn_daemon_timer_;
};

#endif
