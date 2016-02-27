#include <glib-object.h>
#include <cstdio>
#include <cstdlib>
#include <QTimer>
#include <QStringList>
#include <QString>
#include <QDebug>
#include <QDir>
#include <QCoreApplication>

extern "C" {
#include <ccnet/ccnet-client.h>
}


#include "utils/utils.h"
#include "utils/process.h"
#include "configurator.h"
#include "seafile-applet.h"
#include "rpc/rpc-client.h"
#include "daemon-mgr.h"

namespace {

const int kConnDaemonIntervalMilli = 1000;

#if defined(Q_OS_WIN32)
const char *kCcnetDaemonExecutable = "ccnet.exe";
const char *kSeafileDaemonExecutable = "seaf-daemon.exe";
#else
const char *kCcnetDaemonExecutable = "ccnet";
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
    : ccnet_daemon_(0),
    seaf_daemon_(0),
    sync_client_(0)
{
    conn_daemon_timer_ = new QTimer(this);
    connect(conn_daemon_timer_, SIGNAL(timeout()), this, SLOT(tryConnCcnet()));
    shutdown_process (kCcnetDaemonExecutable);

    check_seaf_daemon_ready_timer_ = new QTimer(this);
    connect(check_seaf_daemon_ready_timer_, SIGNAL(timeout()), this, SLOT(checkSeafDaemonReady()));

    system_shut_down_ = false;
    connect(qApp, SIGNAL(aboutToQuit()),
            this, SLOT(systemShutDown()));
}

DaemonManager::~DaemonManager() {
    stopAllDaemon();
}

void DaemonManager::startCcnetDaemon()
{
    sync_client_ = ccnet_client_new();

    const QString config_dir = seafApplet->configurator()->ccnetDir();
    const QByteArray path = config_dir.toUtf8();
    if (ccnet_client_load_confdir(sync_client_, NULL, path.data()) <  0) {
        seafApplet->errorAndExit(tr("failed to load ccnet config dir %1").arg(config_dir));
    }

    ccnet_daemon_ = new QProcess(this);
    connect(ccnet_daemon_, SIGNAL(started()), this, SLOT(onCcnetDaemonStarted()));
    connect(ccnet_daemon_, SIGNAL(finished(int, QProcess::ExitStatus)),
            this, SLOT(onCcnetDaemonExited()));

    QStringList args;
    args << "-c" << config_dir;
    ccnet_daemon_->start(RESOURCE_PATH(kCcnetDaemonExecutable), args);
    qWarning() << "starting ccnet: " << args;
}

void DaemonManager::startSeafileDaemon()
{
    const QString config_dir = seafApplet->configurator()->ccnetDir();
    const QString seafile_dir = seafApplet->configurator()->seafileDir();
    const QString worktree_dir = seafApplet->configurator()->worktreeDir();

    seaf_daemon_ = new QProcess(this);
    connect(seaf_daemon_, SIGNAL(started()), this, SLOT(onSeafDaemonStarted()));
    connect(seaf_daemon_, SIGNAL(finished(int, QProcess::ExitStatus)),
            this, SLOT(onSeafDaemonExited()));

    QStringList args;
    args << "-c" << config_dir << "-d" << seafile_dir << "-w" << worktree_dir;
    seaf_daemon_->start(RESOURCE_PATH(kSeafileDaemonExecutable), args);
    qWarning() << "starting seaf-daemon: " << args;
}

void DaemonManager::onCcnetDaemonStarted()
{
    conn_daemon_timer_->start(kConnDaemonIntervalMilli);
}

void DaemonManager::systemShutDown()
{
    system_shut_down_ = true;
}

void DaemonManager::onSeafDaemonStarted()
{
    qDebug("seafile daemon is now running, checking if the service is ready");
    check_seaf_daemon_ready_timer_->start(kConnDaemonIntervalMilli);
}

void DaemonManager::checkSeafDaemonReady()
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

void DaemonManager::onCcnetDaemonExited()
{
    // if (!system_shut_down_) {
    //     seafApplet->errorAndExit(tr("ccnet daemon has exited abnormally"));
    // }
}

void DaemonManager::onSeafDaemonExited()
{
    // if (!system_shut_down_) {
    //     seafApplet->errorAndExit(tr("seafile daemon has exited abnormally"));
    // }
}

void DaemonManager::stopAllDaemon()
{
    qWarning("[Daemon Mgr] stopping ccnet/seafile daemon");

    if (conn_daemon_timer_)
        conn_daemon_timer_->stop();
    if (check_seaf_daemon_ready_timer_) {
        check_seaf_daemon_ready_timer_->stop();
    }
    if (seaf_daemon_) {
        seaf_daemon_->kill();
        seaf_daemon_->waitForFinished(50);
    }
    if (ccnet_daemon_) {
        ccnet_daemon_->kill();
        ccnet_daemon_->waitForFinished(10);
    }
}

void DaemonManager::tryConnCcnet()
{
    qWarning("trying to connect to ccnet daemon...\n");

    if (ccnet_client_connect_daemon(sync_client_, CCNET_CLIENT_SYNC) < 0) {
        return;
    } else {
        conn_daemon_timer_->stop();

        qWarning("connected to ccnet daemon\n");

        startSeafileDaemon();
    }
}
