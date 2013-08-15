#include <glib-object.h>
#include <QTimer>
#include <QSocketNotifier>
#include <QStringList>
#include <QString>

extern "C" {
#include <ccnet/ccnet-client.h>
}


#include "daemon-mgr.h"
#include "configurator.h"
#include "seafile-applet.h"

namespace {

const int kCheckIntervalMilli = 5000;
const int kConnDaemonIntervalMilli = 1000;

} // namespace


DaemonManager::DaemonManager()
    : sync_client_(0),
      socket_notifier_(0),
      ccnet_daemon_(0),
      seaf_daemon_(0),
      sync_client_(0)
{
    monitor_timer_ = new QTimer(this);
    conn_daemon_timer_ = new QTimer(this);
    connect(conn_daemon_timer_, SIGNAL(timeout()), this, SLOT(tryConnCcnet()));
}

void DaemonManager::startCcnetDaemon()
{
    if (sync_client_ != 0) {
        g_object_unref (sync_client_);
    }
    if (ccnet_daemon_) {
        ccnet_daemon_->terminate();
        delete ccnet_daemon_;
    }
    sync_client_ = ccnet_client_new();

    const QString config_dir = seafApplet->configurator()->ccnetDir();
    const QByteArray path = config_dir.toUtf8();
    if (ccnet_client_load_confdir(sync_client_, path.data()) <  0) {
        seafApplet->errorAndExit(tr("failed to load ccnet config dir %1").arg(config_dir));
    }

    ccnet_daemon_ = new QProcess(this);
    connect(ccnet_daemon_, SIGNAL(stateChanged(QProcess::ProcessState)), this, SLOT(ccnetStateChanged(QProcess::ProcessState)));
    QStringList args;
    args.push_back(QString("-c"));
    args.push_back(config_dir);
    args.push_back(QString("-D"));
    args.push_back(QString("Peer,Message,Connection,Other"));
    ccnet_daemon_->start(QString("ccnet"), args);
}

void DaemonManager::startSeafileDaemon()
{
    if (seaf_daemon_) {
        seaf_daemon_->terminate();
        delete seaf_daemon_;
    }
    const QString config_dir = seafApplet->configurator()->ccnetDir();
    const QString seafile_dir = seafApplet->configurator()->seafileDir();
    const QString worktree_dir = seafApplet->configurator()->worktreeDir();

    seaf_daemon_ = new QProcess(this);
    connect(seaf_daemon_, SIGNAL(stateChanged(QProcess::ProcessState)), this, SLOT(seafStateChanged(QProcess::ProcessState)));
    QStringList args;
    args.push_back(QString("-c"));
    args.push_back(config_dir);
    args.push_back(QString("-d"));
    args.push_back(seafile_dir);
    args.push_back(QString("-w"));
    args.push_back(worktree_dir);
    seaf_daemon_->start(QString("seaf-daemon"), args);
}

void DaemonManager::ccnetStateChanged(QProcess::ProcessState state)
{
    if (state == QProcess::Running) {
        qDebug("ccnet Running");
        conn_daemon_timer_->start(kConnDaemonIntervalMilli);
    } else if (state == QProcess::NotRunning) {
        qDebug("ccnet NotRunning");
        startCcnetDaemon();
        emit ccnetDaemonDisconnected();
    }
}

void DaemonManager::seafStateChanged(QProcess::ProcessState state)
{
    if (state == QProcess::Running) {
        qDebug("seaf-daemon Running");
    } else if (state == QProcess::NotRunning) {
        qDebug("seaf-daemon NotRunning");
        if (ccnet_daemon_ && ccnet_daemon_->state() == QProcess::Running) {
            startSeafileDaemon();
        }
    }
}

void DaemonManager::stopAll()
{
    if (seaf_daemon_)
        seaf_daemon_->terminate();
    if (ccnet_daemon_)
        ccnet_daemon_->terminate();
}

void DaemonManager::tryConnCcnet()
{
    qDebug("trying to connect to ccnet daemon...\n");

    if (ccnet_client_connect_daemon(sync_client_, CCNET_CLIENT_SYNC) < 0) {
        return;
    } else {
        qDebug("connected to ccnet daemon\n");
        conn_daemon_timer_->stop();

        startSocketNotifier();
        startSeafileDaemon();

        emit ccnetDaemonConnected();
    }
}

void DaemonManager::startSocketNotifier()
{
    if (socket_notifier_ != 0) {
        delete socket_notifier_;
    }
    socket_notifier_ = new QSocketNotifier(sync_client_->connfd,
                                           QSocketNotifier::Read);

    // The socket notification is used to detect the disconnect of the sync
    // client's socket. (Since we never use the sync client directly, the Read
    // event must be an EOF)
    connect(socket_notifier_, SIGNAL(activated(int)), this, SLOT(ccnetDaemonDown()));
}

void DaemonManager::ccnetDaemonDown()
{
    socket_notifier_->setEnabled(false);
    seafApplet->errorAndExit(tr("Seafile client has encountered an internal error."));
}
