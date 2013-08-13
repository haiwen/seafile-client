#include <glib-object.h>
#include <QTimer>

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


DaemonManager::DaemonManager(QObject *parent)
    : QObject(parent),
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
    sync_client_ = ccnet_client_new();

    const QString config_dir = seafApplet->configurator()->ccnetDir();
    const QByteArray path = config_dir.toUtf8();
    if (ccnet_client_load_confdir(sync_client_, path.data()) <  0) {
        seafApplet->errorAndExit(tr("failed to load ccnet config dir %1").arg(config_dir));
    }

    // TODO: start ccnet processs using QProcess
    conn_daemon_timer_->start(kConnDaemonIntervalMilli);
}

void DaemonManager::tryConnCcnet()
{
    qDebug("trying to connect to ccnet daemon...\n");

    if (ccnet_client_connect_daemon(sync_client_, CCNET_CLIENT_SYNC) < 0) {
        return;
    } else {
        qDebug("connected to ccnet daemon\n");
        conn_daemon_timer_->stop();
        emit ccnetDaemonConnected();
    }
}
