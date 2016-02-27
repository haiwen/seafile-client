#ifndef SEAFILE_CLIENT_DAEMON_MANAGER_H
#define SEAFILE_CLIENT_DAEMON_MANAGER_H

#include <QObject>
#include <QProcess>

struct _CcnetClient;

class QTimer;

/**
 * Start/Monitor ccnet/seafile daemon
 */
class DaemonManager : public QObject {
    Q_OBJECT

public:
    DaemonManager();
    ~DaemonManager();
    void startCcnetDaemon();

signals:
    void daemonStarted();

private slots:
    void tryConnCcnet();
    void onCcnetDaemonStarted();
    void onCcnetDaemonExited();
    void onSeafDaemonStarted();
    void onSeafDaemonExited();
    void systemShutDown();
    void checkSeafDaemonReady();

private:
    Q_DISABLE_COPY(DaemonManager)

    void startSeafileDaemon();
    void stopAllDaemon();

    QTimer *conn_daemon_timer_;
    QProcess *ccnet_daemon_;
    QProcess *seaf_daemon_;
    _CcnetClient *sync_client_;

    QTimer *check_seaf_daemon_ready_timer_;

    bool system_shut_down_;
};

#endif // SEAFILE_CLIENT_DAEMON_MANAGER_H
