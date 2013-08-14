#ifndef SEAFILE_CLIENT_DAEMON_MANAGER_H
#define SEAFILE_CLIENT_DAEMON_MANAGER_H

#include <QObject>
#include <QProcess>

class QTimer;

struct _CcnetClient;

/**
 * Start/Stop/Monitor ccnet/seafile daemon
 */
class DaemonManager : public QObject {
    Q_OBJECT

public:
    DaemonManager();
    void startCcnetDaemon();
    void startSeafileDaemon();
    void stopAll();

signals:
    void ccnetDaemonConnected();
    void ccnetDaemonDisconnected();

private slots:
    void tryConnCcnet();
    void ccnetStateChanged(QProcess::ProcessState state);
    void seafStateChanged(QProcess::ProcessState state);

private:
    Q_DISABLE_COPY(DaemonManager)

    QTimer *monitor_timer_;
    QTimer *conn_daemon_timer_;
    QProcess *ccnet_daemon_;
    QProcess *seaf_daemon_;
    _CcnetClient *sync_client_;
};

#endif // SEAFILE_CLIENT_DAEMON_MANAGER_H
