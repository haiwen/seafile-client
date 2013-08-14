#ifndef SEAFILE_CLIENT_DAEMON_MANAGER_H
#define SEAFILE_CLIENT_DAEMON_MANAGER_H

#include <QObject>

struct _CcnetClient;

class QTimer;
class QSocketNotifier;

/**
 * Start/Monitor ccnet/seafile daemon
 */
class DaemonManager : public QObject {
    Q_OBJECT

public:
    DaemonManager();
    void startCcnetDaemon();

signals:
    void ccnetDaemonConnected();

private slots:
    void tryConnCcnet();
    void ccnetDaemonDown();

private:
    Q_DISABLE_COPY(DaemonManager)

    void startSocketNotifier();
    // void startMonitor();
    // void stopMonitor();

    QTimer *monitor_timer_;
    QTimer *conn_daemon_timer_;

    _CcnetClient *sync_client_;

    QSocketNotifier *socket_notifier_;
};

#endif // SEAFILE_CLIENT_DAEMON_MANAGER_H
