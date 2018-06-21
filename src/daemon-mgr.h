#ifndef SEAFILE_CLIENT_DAEMON_MANAGER_H
#define SEAFILE_CLIENT_DAEMON_MANAGER_H

#include <QObject>
#include <QProcess>

class QTimer;

/**
 * Start/Monitor seafile daemon
 */
class DaemonManager : public QObject {
    Q_OBJECT

public:
    DaemonManager();
    ~DaemonManager();
    void startSeafileDaemon();

signals:
    void daemonStarted();

private slots:
    void onDaemonStarted();
    void onDaemonExited();
    void systemShutDown();
    void checkDaemonReady();

private:
    Q_DISABLE_COPY(DaemonManager)

    void stopDaemon();

    QProcess *seaf_daemon_;
    QTimer *check_seaf_daemon_ready_timer_;

    bool system_shut_down_;
};

#endif // SEAFILE_CLIENT_DAEMON_MANAGER_H
