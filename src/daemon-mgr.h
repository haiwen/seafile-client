#ifndef SEAFILE_CLIENT_DAEMON_MANAGER_H
#define SEAFILE_CLIENT_DAEMON_MANAGER_H

#include <QObject>

class QTimer;
struct _CcnetClient;

/**
 * Start/Stop/Monitor ccnet/seafile daemon
 */
class DaemonManager : public QObject {
    Q_OBJECT

public:
    DaemonManager(QObject *parent=0);
    void startCcnetDaemon();
    // void stopAll();

    // void startSeafileDaemon();

    // void startMonitor();
    // void stopMonitor();

signals:
    void ccnetDaemonConnected();
    void ccnetDaemonDisconnected();

private slots:
    void tryConnCcnet();

private:

    QTimer *monitor_timer_;
    QTimer *conn_daemon_timer_;

    _CcnetClient *sync_client_;
};

#endif // SEAFILE_CLIENT_DAEMON_MANAGER_H
