#include <glib-object.h>
#include <cstdio>
#include <cstdlib>
#include <QTimer>
#include <QStringList>
#include <QString>
#include <QDebug>
#include <QDir>
#include <QCoreApplication>

#include "utils/utils.h"
#include "utils/process.h"
#include "configurator.h"
#include "seafile-applet.h"
#include "rpc/rpc-client.h"
#include "daemon-mgr.h"

namespace {

const int kConnDaemonIntervalMilli = 1000;
const int kMaxDaemonReadyCheck = 15;

const int kDaemonRestartInternvalMSecs = 2000;
const int kDaemonRestartMaxRetries = 10;

#if defined(Q_OS_WIN32)
const char *kSeafileDaemonExecutable = "seaf-daemon.exe";
#else
const char *kSeafileDaemonExecutable = "seaf-daemon";
#endif

typedef enum {
    DAEMON_INIT = 0,
    DAEMON_STARTING,
    DAEMON_CONNECTING,
    DAEMON_CONNECTED,
    DAEMON_DEAD,
    SEAFILE_EXITING,
    MAX_STATE,
} DaemonState;

const char *DaemonStateStrs[] = {
    "init",
    "starting",
    "connecting",
    "connected",
    "dead",
    "seafile_exiting"
};

const char *stateToStr(int state)
{
    if (state < 0 || state >= MAX_STATE) {
        return "";
    }
    return DaemonStateStrs[state];
}

bool seafileRpcReady() {
    SeafileRpcClient rpc;
    if (!rpc.tryConnectDaemon()) {
        return false;
    }

    QString str;
    return rpc.seafileGetConfig("use_proxy", &str) == 0;
}


} // namespace



DaemonManager::DaemonManager()
    : seaf_daemon_(nullptr)
{
    current_state_ = DAEMON_INIT;
    first_start_ = true;
    restart_retried_ = 0;

    conn_daemon_timer_ = new QTimer(this);
    connect(conn_daemon_timer_, SIGNAL(timeout()), this, SLOT(checkDaemonReady()));

    connect(qApp, SIGNAL(aboutToQuit()),
            this, SLOT(systemShutDown()));
}

DaemonManager::~DaemonManager() {
    stopDaemon();
}

void DaemonManager::restartSeafileDaemon()
{
    if (current_state_ == SEAFILE_EXITING) {
        return;
    }

    qWarning("Trying to restart seafile daemon");
    startSeafileDaemon();
}

void DaemonManager::startSeafileDaemon()
{
    seaf_daemon_ = new QProcess(this);
    connect(seaf_daemon_, SIGNAL(started()), this, SLOT(onDaemonStarted()));
    connect(seaf_daemon_, SIGNAL(finished(int, QProcess::ExitStatus)),
            SLOT(onDaemonFinished(int, QProcess::ExitStatus)));

    const QString config_dir = seafApplet->configurator()->ccnetDir();
    const QString seafile_dir = seafApplet->configurator()->seafileDir();
    const QString worktree_dir = seafApplet->configurator()->worktreeDir();

    QStringList args;
    args << "-c" << config_dir << "-d" << seafile_dir << "-w" << worktree_dir;
    seaf_daemon_->start(RESOURCE_PATH(kSeafileDaemonExecutable), args);
    qWarning() << "starting seaf-daemon: " << args;
    transitionState(DAEMON_STARTING);
}

void DaemonManager::systemShutDown()
{
    transitionState(SEAFILE_EXITING);
}

void DaemonManager::onDaemonStarted()
{
    qDebug("seafile daemon is now running, checking if the service is ready");
    conn_daemon_timer_->start(kConnDaemonIntervalMilli);
    transitionState(DAEMON_CONNECTING);
}

void DaemonManager::checkDaemonReady()
{
    QString str;
    // Because some settings need to be loaded from seaf daemon, we only emit
    // the "daemonStarted" signal after we're sure the daemon rpc is ready.
    if (seafileRpcReady()) {
        qDebug("seaf daemon is ready");
        conn_daemon_timer_->stop();
        transitionState(DAEMON_CONNECTED);
        restart_retried_ = 0;
        if (first_start_) {
            first_start_ = false;
            emit daemonStarted();
        } else {
            emit daemonRestarted();
        }
        return;
    }
    qDebug("seaf daemon is not ready");
    static int maxcheck = 0;
    if (++maxcheck > kMaxDaemonReadyCheck) {
        qWarning("seafile rpc is not ready after %d retry, abort", maxcheck);
        seafApplet->errorAndExit(tr("%1 client failed to initialize").arg(getBrand()));
    }
}

void DaemonManager::onDaemonFinished(int exit_code, QProcess::ExitStatus exit_status)
{
    qWarning("Seafile daemon process %s with code %d ",
             (current_state_ != SEAFILE_EXITING &&
              exit_status == QProcess::CrashExit)
                 ? "crashed"
                 : "exited normally",
             exit_code);


    if (current_state_ == DAEMON_CONNECTING) {
        conn_daemon_timer_->stop();
        scheduleRestartDaemon();
    } else if (current_state_ != SEAFILE_EXITING) {
        transitionState(DAEMON_DEAD);
        emit daemonDead();
        scheduleRestartDaemon();
    }
}

void DaemonManager::stopDaemon()
{
    conn_daemon_timer_->stop();
    if (seaf_daemon_) {
        qWarning("[Daemon Mgr] stopping seafile daemon");
        // TODO: add an "exit" rpc in seaf-daemon to exit gracefully?
        seaf_daemon_->kill();
        seaf_daemon_->waitForFinished(50);
        seaf_daemon_ = nullptr;
    }
}

void DaemonManager::scheduleRestartDaemon()
{
    // When the daemon crashes when we first start seafile, we should
    // not retry too many times, because during the retry nothing
    // would be shown to the user and would confuse him.
    int max_retry = 2;
    if (seafApplet->rpcClient() && seafApplet->rpcClient()->isConnected()) {
        max_retry = kDaemonRestartMaxRetries;
    }
    if (++restart_retried_ >= max_retry) {
        qWarning("reaching max tries of restarting seafile daemon, aborting");
        seafApplet->errorAndExit(tr("%1 exited unexpectedly").arg(getBrand()));
        return;
    }
    QTimer::singleShot(kDaemonRestartInternvalMSecs, this, SLOT(restartSeafileDaemon()));
}

void DaemonManager::transitionState(int new_state)
{
    qDebug("daemon mgr: %s => %s", stateToStr(current_state_), stateToStr(new_state));
    current_state_ = new_state;
}
