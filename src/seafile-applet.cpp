#include <QtGui>
#include <QApplication>
#include <QDesktopServices>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QCoreApplication>
#include <QMessageBox>
#include <QTimer>

#include <glib.h>

#include "utils/utils.h"
#include "utils/log.h"
#include "account-mgr.h"
#include "configurator.h"
#include "daemon-mgr.h"
#include "message-listener.h"
#include "settings-mgr.h"
#include "certs-mgr.h"
#include "rpc/rpc-client.h"
#include "ui/main-window.h"
#include "ui/tray-icon.h"
#include "ui/settings-dialog.h"
#include "ui/init-vdrive-dialog.h"
#include "ui/login-dialog.h"
#include "open-local-helper.h"
#include "repo-service.h"
#include "events-service.h"
#include "avatar-service.h"
#include "seahub-notifications-monitor.h"

#include "seafile-applet.h"

namespace {

void myLogHandler(QtMsgType type, const char *msg)
{
    switch (type) {
    case QtDebugMsg:
        g_debug("%s", msg);
        break;
    case QtWarningMsg:
        g_warning("%s", msg);
        break;
    case QtCriticalMsg:
        g_critical("%s", msg);
        break;
    case QtFatalMsg:
        g_critical("%s", msg);
        abort();
    }
}

/**
 * s1 > s2 --> *ret = 1
 *    = s2 --> *ret = 0
 *    < s2 --> *ret = -1
 *
 * If any of the vesion strings is invalid, return -1; else return 0
 */
int compareVersions(const QString& s1, const QString& s2, int *ret)
{
    QStringList v1 = s1.split(".");
    QStringList v2 = s2.split(".");

    int i = 0;
    while (i < v1.size() && i < v2.size()) {
        bool ok;
        int a = v1[i].toInt(&ok);
        if (!ok) {
            return -1;
        }
        int b = v2[i].toInt(&ok);
        if (!ok) {
            return -1;
        }

        if (a > b) {
            *ret = 1;
            return 0;
        } else if (a < b) {
            *ret = -1;
            return 0;
        }

        i++;
    }

    *ret = v1.size() - v2.size();

    return 0;
}

const int kIntervalBeforeShowInitVirtualDialog = 3000;

const char *kSeafileClientDownloadUrl = "http://seafile.com/en/download/";
const char *kSeafileClientDownloadUrlChinese = "http://seafile.com/download/";

} // namespace


SeafileApplet *seafApplet;

SeafileApplet::SeafileApplet()
    : configurator_(new Configurator),
      account_mgr_(new AccountManager),
      daemon_mgr_(new DaemonManager),
      main_win_(NULL),
      rpc_client_(new SeafileRpcClient),
      message_listener_(new MessageListener),
      settings_dialog_(new SettingsDialog),
      settings_mgr_(new SettingsManager),
      certs_mgr_(new CertsManager),
      started_(false),
      in_exit_(false)
{
    tray_icon_ = new SeafileTrayIcon(this);
    init_seafile_hide_dock_icon();
}

void SeafileApplet::start()
{
    refreshQss();

    configurator_->checkInit();

    initLog();

    account_mgr_->start();

    certs_mgr_->start();

    RepoService::instance()->start();
    EventsService::instance()->start();
    AvatarService::instance()->start();
    SeahubNotificationsMonitor::instance()->start();

#if defined(Q_WS_WIN)
    QString crash_rpt_path = QDir(configurator_->ccnetDir()).filePath("logs/seafile-crash-report.txt");
    if (!g_setenv ("CRASH_RPT_PATH", toCStr(crash_rpt_path), FALSE))
        qDebug("Failed to set CRASH_RPT_PATH env variable.\n");
#endif

    daemon_mgr_->startCcnetDaemon();

    connect(daemon_mgr_, SIGNAL(daemonStarted()),
            this, SLOT(onDaemonStarted()));
}

void SeafileApplet::onDaemonStarted()
{
    main_win_ = new MainWindow;

    rpc_client_->connectDaemon();
    message_listener_->connectDaemon();
    seafApplet->settingsManager()->loadSettings();

    if (configurator_->firstUse() || account_mgr_->accounts().size() == 0) {
        LoginDialog login_dialog;
        login_dialog.exec();
    }

    started_ = true;

    if (configurator_->firstUse() || !settings_mgr_->hideMainWindowWhenStarted()) {
        main_win_->showWindow();
    }

    tray_icon_->start();
    tray_icon_->setState(SeafileTrayIcon::STATE_DAEMON_UP);

#if defined(Q_WS_WIN)
    QTimer::singleShot(kIntervalBeforeShowInitVirtualDialog, this, SLOT(checkInitVDrive()));
    configurator_->installCustomUrlHandler();
#endif

    if (settings_mgr_->isCheckLatestVersionEnabled()) {
        checkLatestVersionInfo();
    }

    OpenLocalHelper::instance()->checkPendingOpenLocalRequest();
}

void SeafileApplet::checkInitVDrive()
{
    if (configurator_->firstUse() && account_mgr_->accounts().size() > 0) {
        const Account& account = account_mgr_->accounts()[0];
        InitVirtualDriveDialog *dialog = new InitVirtualDriveDialog(account);
        // Move the dialog to the left of the main window
        int x = main_win_->pos().x() - dialog->rect().width() - 30;
        int y = (QApplication::desktop()->screenGeometry().center() - dialog->rect().center()).y();
        dialog->move(qMax(0, x), y);
        dialog->show();
        dialog->raise();
        dialog->activateWindow();
    }
}

void SeafileApplet::exit(int code)
{
    // Must use the global namespace, or the "exit" would call itself util
    // stack overflow
    daemon_mgr_->stopAll();
    // Remove tray icon from system tray
    delete tray_icon_;
    if (main_win_) {
        main_win_->writeSettings();
    }
    ::exit(code);
}

void SeafileApplet::errorAndExit(const QString& error)
{
    if (in_exit_) {
        return;
    }

    in_exit_ = true;

    warningBox(error);
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

bool SeafileApplet::loadQss(const QString& path)
{
    QFile file(path);
    if (!QFileInfo(file).exists()) {
        return false;
    }
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream input(&file);
    style_ += "\n";
    style_ += input.readAll();
    qApp->setStyleSheet(style_);

    return true;
}

void SeafileApplet::refreshQss()
{
    style_.clear();
    loadQss("qt.css") || loadQss(":/qt.css");

#if defined(Q_WS_WIN)
    loadQss("qt-win.css") || loadQss(":/qt-win.css");
#elif defined(Q_WS_X11)
    loadQss("qt-linux.css") || loadQss(":/qt-linux.css");
#else
    loadQss("qt-mac.css") || loadQss(":/qt-mac.css");
#endif
}

void SeafileApplet::warningBox(const QString& msg, QWidget *parent)
{
    QMessageBox::warning(parent != 0 ? parent : main_win_,
                         getBrand(), msg, QMessageBox::Ok);
}

void SeafileApplet::messageBox(const QString& msg, QWidget *parent)
{
    QMessageBox::information(parent != 0 ? parent : main_win_,
                             getBrand(), msg, QMessageBox::Ok);
}

bool SeafileApplet::yesOrNoBox(const QString& msg, QWidget *parent, bool default_val)
{
    QMessageBox::StandardButton default_btn = default_val ? QMessageBox::Yes : QMessageBox::No;

    return QMessageBox::question(parent != 0 ? parent : main_win_,
                                 getBrand(),
                                 msg,
                                 QMessageBox::Yes | QMessageBox::No,
                                 default_btn) == QMessageBox::Yes;
}

bool SeafileApplet::detailedYesOrNoBox(const QString& msg, const QString& detailed_text, QWidget *parent, bool default_val)
{
    QMessageBox *msgBox = new QMessageBox(QMessageBox::Question,
                       getBrand(),
                       msg,
                       QMessageBox::Yes | QMessageBox::No,
                       parent != 0 ? parent : main_win_);
    msgBox->setDetailedText(detailed_text);
    // Turns out the layout box in the QMessageBox is a grid
    // You can force the resize using a spacer this way:
    QSpacerItem* horizontalSpacer = new QSpacerItem(400, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
    QGridLayout* layout = (QGridLayout*)msgBox->layout();
layout->addItem(horizontalSpacer, layout->rowCount(), 0, 1, layout->columnCount());
    msgBox->setDefaultButton(default_val ? QMessageBox::Yes : QMessageBox::No);
    return msgBox->exec() == QMessageBox::Yes;
}

void SeafileApplet::checkLatestVersionInfo()
{
    QString id = rpc_client_->getCcnetPeerId();
    QString version = STRINGIZE(SEAFILE_CLIENT_VERSION);

    GetLatestVersionRequest *req = new GetLatestVersionRequest(id, version);
    req->send();

    connect(req, SIGNAL(success(const QString&)),
            this, SLOT(onGetLatestVersionInfoSuccess(const QString&)));
}

void SeafileApplet::onGetLatestVersionInfoSuccess(const QString& latest_version)
{
    QString current_version = STRINGIZE(SEAFILE_CLIENT_VERSION);

    int ret;
    if (compareVersions(current_version, latest_version, &ret) < 0) {
        return;
    }

    if (ret >= 0) {
        return;
    }

    QString msg = tr("A new version of %1 client (%2) is available.\n"
                     "Do you want to visit the download page?").arg(getBrand()).arg(latest_version);

    if (!yesOrNoBox(msg, NULL, true)) {
        return;
    }

    QString url;
    if (QLocale::system().name() == "zh_CN") {
        url = kSeafileClientDownloadUrlChinese;
    } else {
        url = kSeafileClientDownloadUrl;
    }

    QDesktopServices::openUrl(url);
}

QString getBrand()
{
    return QString::fromUtf8(SEAFILE_CLIENT_BRAND);
}
