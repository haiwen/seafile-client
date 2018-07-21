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

    void daemonDead();
    void daemonRestarted();

private slots:
    void onDaemonStarted();
    void onDaemonFinished(int exit_code, QProcess::ExitStatus exit_status);
    void systemShutDown();
    void checkDaemonReady();
    void restartSeafileDaemon();

private:
    Q_DISABLE_COPY(DaemonManager)

    void stopDaemon();
    void scheduleRestartDaemon();
    void transitionState(int new_state);

    QProcess *seaf_daemon_;
    QTimer *conn_daemon_timer_;

    int current_state_;
    // Used to decide whether to emit daemonStarted or daemonRestarted
    bool first_start_;
    int restart_retried_;
};

#endif // SEAFILE_CLIENT_DAEMON_MANAGER_H
