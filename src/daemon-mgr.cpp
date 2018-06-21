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

#if defined(Q_OS_WIN32)
const char *kSeafileDaemonExecutable = "seaf-daemon.exe";
#else
const char *kSeafileDaemonExecutable = "seaf-daemon";
#endif


bool seafileRpcReady() {
    SeafileRpcClient rpc;
    rpc.connectDaemon();

    QString str;
    return rpc.seafileGetConfig("use_proxy", &str) == 0;
}


} // namespace



DaemonManager::DaemonManager()
    : seaf_daemon_(nullptr)
{
    check_seaf_daemon_ready_timer_ = new QTimer(this);
    connect(check_seaf_daemon_ready_timer_, SIGNAL(timeout()), this, SLOT(checkDaemonReady()));

    system_shut_down_ = false;
    connect(qApp, SIGNAL(aboutToQuit()),
            this, SLOT(systemShutDown()));
}

DaemonManager::~DaemonManager() {
    stopDaemon();
}

void DaemonManager::startSeafileDaemon()
{
    seaf_daemon_ = new QProcess(this);
    connect(seaf_daemon_, SIGNAL(started()), this, SLOT(onDaemonStarted()));
    connect(seaf_daemon_, SIGNAL(finished(int, QProcess::ExitStatus)),
            this, SLOT(onDaemonExited()));

    const QString config_dir = seafApplet->configurator()->ccnetDir();
    const QString seafile_dir = seafApplet->configurator()->seafileDir();
    const QString worktree_dir = seafApplet->configurator()->worktreeDir();

    QStringList args;
    args << "-c" << config_dir << "-d" << seafile_dir << "-w" << worktree_dir;
    seaf_daemon_->start(RESOURCE_PATH(kSeafileDaemonExecutable), args);
    qWarning() << "starting seaf-daemon: " << args;
}

void DaemonManager::systemShutDown()
{
    system_shut_down_ = true;
}

void DaemonManager::onDaemonStarted()
{
    qDebug("seafile daemon is now running, checking if the service is ready");
    check_seaf_daemon_ready_timer_->start(kConnDaemonIntervalMilli);
}

void DaemonManager::checkDaemonReady()
{
    QString str;
    // Because some settings need to be loaded from seaf daemon, we only emit
    // the "daemonStarted" signal after we're sure the daemon rpc is ready.
    if (seafileRpcReady()) {
        qDebug("seaf daemon is ready");
        check_seaf_daemon_ready_timer_->stop();
        emit daemonStarted();
        return;
    }
    qDebug("seaf daemon is not ready");
    static int maxcheck = 0;
    if (++maxcheck > 15) {
        qWarning("seafile rpc is not ready after %d retry, abort", maxcheck);
        seafApplet->errorAndExit(tr("%1 client failed to initialize").arg(getBrand()));
    }
}

void DaemonManager::onDaemonExited()
{
    // if (!system_shut_down_) {
    //     seafApplet->errorAndExit(tr("seafile daemon has exited abnormally"));
    // }
}

void DaemonManager::stopDaemon()
{
    check_seaf_daemon_ready_timer_->stop();
    if (seaf_daemon_) {
        qWarning("[Daemon Mgr] stopping seafile daemon");
        seaf_daemon_->kill();
        seaf_daemon_->waitForFinished(50);
        seaf_daemon_ = nullptr;
    }
}
