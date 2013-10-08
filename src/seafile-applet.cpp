#include <QMessageBox>
#include <QDir>

#include <glib.h>

#include "utils/utils.h"
#include "utils/log.h"
#include "account-mgr.h"
#include "configurator.h"
#include "daemon-mgr.h"
#include "message-listener.h"
#include "settings-mgr.h"
#include "rpc/rpc-client.h"
#include "ui/main-window.h"
#include "ui/tray-icon.h"
#include "ui/settings-dialog.h"
#include "ui/login-dialog.h"
#include "seafile-applet.h"

namespace {

void myLogHandler(QtMsgType type, const char *msg)
{
    switch (type) {
    case QtDebugMsg:
        g_debug("%s\n", msg);
        break;
    case QtWarningMsg:
        g_warning("%s\n", msg);
        break;
    case QtCriticalMsg:
        g_critical("%s\n", msg);
        break;
    case QtFatalMsg:
        g_critical("%s\n", msg);
        abort();
    }
}


} // namespace

SeafileApplet *seafApplet;

SeafileApplet::SeafileApplet()
    : configurator_(new Configurator),
      account_mgr_(new AccountManager),
      daemon_mgr_(new DaemonManager),
      rpc_client_(new SeafileRpcClient),
      message_listener_(new MessageListener),
      settings_dialog_(new SettingsDialog),
      settings_mgr_(new SettingsManager),
      in_exit_(false)
{
    tray_icon_ = new SeafileTrayIcon(this);
}

void SeafileApplet::start()
{
    configurator_->checkInit();

    tray_icon_->show();

    initLog();

    account_mgr_->start();

#if defined(Q_WS_WIN)
    QString crash_rpt_path = QDir(configurator_->ccnetDir()).filePath("logs/seafile-crash-report.txt");
    if (!g_setenv ("CRASH_RPT_PATH", toCStr(crash_rpt_path), FALSE))
        qDebug("Failed to set CRASH_RPT_PATH env variable.\n");
#endif

    if (configurator_->firstUse()) {
        LoginDialog dialog;
        dialog.exec();
    }

    daemon_mgr_->startCcnetDaemon();

    connect(daemon_mgr_, SIGNAL(daemonStarted()),
            this, SLOT(onDaemonStarted()));
}

void SeafileApplet::onDaemonStarted()
{
    // tray_icon_->notify("Seafile", "daemon is started");
    main_win_ = new MainWindow;

    main_win_->showWindow();
    tray_icon_->setState(SeafileTrayIcon::STATE_DAEMON_UP);

    rpc_client_->connectDaemon();
    message_listener_->connectDaemon();
    seafApplet->settingsManager()->loadSettings();
}

void SeafileApplet::exit(int code)
{
    // Must use the global namespace, or the "exit" would call itself util
    // stack overflow
    daemon_mgr_->stopAll();
    // Remove tray icon from system tray
    delete tray_icon_;
    ::exit(code);
}

void SeafileApplet::errorAndExit(const QString& error)
{
    in_exit_ = true;
    QMessageBox::warning(main_win_, tr("Seafile"), error, QMessageBox::Ok);
    this->exit(1);
}

void SeafileApplet::initLog()
{
    if (applet_log_init(toCStr(configurator_->ccnetDir())) < 0) {
        errorAndExit(tr("Failed to initialize log"));
    } else {
        qInstallMsgHandler(myLogHandler);
    }
}

