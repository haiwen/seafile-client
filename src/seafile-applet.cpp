#include <QtGlobal>

#include <QtWidgets>
#include <QApplication>
#include <QDesktopServices>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QCoreApplication>
#include <QMessageBox>
#include <QTimer>
#include <QHostInfo>

#include <errno.h>
#include <glib.h>

#include "utils/utils.h"
#include "utils/file-utils.h"
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
#include "avatar-service.h"
#include "filebrowser/thumbnail-service.h"
#include "seahub-notifications-monitor.h"
#include "filebrowser/data-cache.h"
#include "filebrowser/auto-update-mgr.h"
#include "rpc/local-repo.h"
#include "network-mgr.h"
#include "server-status-service.h"
#include "account-info-service.h"
#include "customization-service.h"

#if defined(Q_OS_WIN32)
    #include "ext-handler.h"
    #include "utils/registry.h"
#elif defined(HAVE_FINDER_SYNC_SUPPORT)
    #include "finder-sync/finder-sync-listener.h"
#endif

#ifdef HAVE_SPARKLE_SUPPORT
#include "auto-update-service.h"
#endif


#if defined(Q_OS_MAC)
#include "utils/utils-mac.h"
#endif

#include "seafile-applet.h"

namespace {
enum DEBUG_LEVEL {
  DEBUG = 0,
  WARNING
};

// -DQT_NO_DEBUG is used with cmake and qmake if it is a release build
// if it is debug build, use DEBUG level as default
#if !defined(QT_NO_DEBUG) || !defined(NDEBUG)
DEBUG_LEVEL seafile_client_debug_level = DEBUG;
#else
// if it is release build, use WARNING level as default
DEBUG_LEVEL seafile_client_debug_level = WARNING;
#endif

void myLogHandlerDebug(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QByteArray localMsg = msg.toLocal8Bit();
    switch (type) {
// Note: By default, this information (QMessageLogContext) is recorded only in debug builds.
// You can overwrite this explicitly by defining QT_MESSAGELOGCONTEXT or QT_NO_MESSAGELOGCONTEXT.
// from http://doc.qt.io/qt-5/qmessagelogcontext.html
#ifdef QT_MESSAGELOGCONTEXT
    case QtDebugMsg:
        g_debug("%s (%s:%u)\n", localMsg.constData(), context.file, context.line);
        break;
    case QtWarningMsg:
        g_warning("%s (%s:%u)\n", localMsg.constData(), context.file, context.line);
        break;
    case QtCriticalMsg:
        g_critical("%s (%s:%u)\n", localMsg.constData(), context.file, context.line);
        break;
    case QtFatalMsg:
        g_critical("%s (%s:%u)\n", localMsg.constData(), context.file, context.line);
        abort();
#else // QT_MESSAGELOGCONTEXT
    case QtDebugMsg:
        g_debug("%s\n", localMsg.constData());
        break;
    case QtWarningMsg:
        g_warning("%s\n", localMsg.constData());
        break;
    case QtCriticalMsg:
        g_critical("%s\n", localMsg.constData());
        break;
    case QtFatalMsg:
        g_critical("%s\n", localMsg.constData());
        abort();
#endif // QT_MESSAGELOGCONTEXT
    default:
        break;
    }
}
void myLogHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QByteArray localMsg = msg.toLocal8Bit();
    switch (type) {
#ifdef QT_MESSAGELOGCONTEXT
    case QtWarningMsg:
        g_warning("%s (%s:%u)\n", localMsg.constData(), context.file, context.line);
        break;
    case QtCriticalMsg:
        g_critical("%s (%s:%u)\n", localMsg.constData(), context.file, context.line);
        break;
    case QtFatalMsg:
        g_critical("%s (%s:%u)\n", localMsg.constData(), context.file, context.line);
        abort();
#else // QT_MESSAGELOGCONTEXT
    case QtWarningMsg:
        g_warning("%s\n", localMsg.constData());
        break;
    case QtCriticalMsg:
        g_critical("%s\n", localMsg.constData());
        break;
    case QtFatalMsg:
        g_critical("%s\n", localMsg.constData());
        abort();
#endif // QT_MESSAGELOGCONTEXT
    default:
        break;
    }
}

#ifdef Q_OS_MAC
void writeCABundleForCurl()
{
    QString ca_bundle_path = QDir(seafApplet->configurator()->seafileDir()).filePath("ca-bundle.pem");
    QFile bundle(ca_bundle_path);
    if (bundle.exists()) {
        bundle.remove();
    }
    bundle.open(QIODevice::WriteOnly);
    const std::vector<QByteArray> certs = utils::mac::getSystemCaCertificates();
    for (size_t i = 0; i < certs.size(); i++) {
        QList<QSslCertificate> list = QSslCertificate::fromData(certs[i], QSsl::Der);
        foreach (const QSslCertificate& cert, list) {
            bundle.write(cert.toPem());
        }
    }
}
#endif


const char *const kPreconfigureUsername = "PreconfigureUsername";
const char *const kPreconfigureUserToken = "PreconfigureUserToken";
const char *const kPreconfigureServerAddr = "PreconfigureServerAddr";
const char *const kPreconfigureComputerName = "PreconfigureComputerName";
const char* const kHideConfigurationWizard = "HideConfigurationWizard";
#if defined(Q_OS_WIN32)
const char *const kSeafileConfigureFileName = "seafile.ini";
const char *const kSeafileConfigurePath = "SOFTWARE\\Seafile";
const int kIntervalBeforeShowInitVirtualDialog = 3000;
#else
const char *const kSeafileConfigureFileName = ".seafilerc";
#endif
const char *const kSeafilePreconfigureGroupName = "preconfigure";

const int kIntervalForUpdateRepoProperty = 1000;

const char *kRepoServerUrlProperty = "server-url";
const char *kRepoRelayAddrProperty = "relay-address";

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
      in_exit_(false),
      is_pro_(false)
{
    tray_icon_ = new SeafileTrayIcon(this);
    connect(qApp, SIGNAL(aboutToQuit()), this, SLOT(onAboutToQuit()));
}

SeafileApplet::~SeafileApplet()
{
    NetworkStatusDetector::instance()->stop();

#ifdef HAVE_FINDER_SYNC_SUPPORT
    finderSyncListenerStop();
#endif
    delete tray_icon_;
    delete certs_mgr_;
    delete settings_dialog_;
    delete message_listener_;
    delete rpc_client_;
    delete daemon_mgr_;

    delete account_mgr_;
    delete configurator_;
    if (main_win_)
        delete main_win_;
#if defined(Q_OS_WIN32)
    SeafileExtensionHandler::instance()->stop();

#ifdef HAVE_SPARKLE_SUPPORT
    AutoUpdateService::instance()->stop();
#endif

#endif
}

void SeafileApplet::start()
{
    refreshQss();

    configurator_->checkInit();

    initLog();

    account_mgr_->start();

    certs_mgr_->start();

#if defined(Q_OS_WIN32)
    QString crash_rpt_path = QDir(configurator_->ccnetDir()).filePath("logs/seafile-crash-report.txt");
    if (!g_setenv ("CRASH_RPT_PATH", toCStr(crash_rpt_path), FALSE))
        qWarning("Failed to set CRASH_RPT_PATH env variable.\n");
#endif

#if defined(Q_OS_MAC)
    writeCABundleForCurl();
#endif

    // Load system proxy information. This must be done before we start
    // seaf-daemon.
    settings_mgr_->writeSystemProxyInfo(
        account_mgr_->currentAccount().serverUrl,
        QDir(configurator_->seafileDir()).filePath("system-proxy.txt"));

    //
    // start daemons
    //
    daemon_mgr_->startCcnetDaemon();

    connect(daemon_mgr_, SIGNAL(daemonStarted()),
            this, SLOT(onDaemonStarted()));
}

void SeafileApplet::onDaemonStarted()
{
    //
    // start daemon-related services
    //
    rpc_client_->connectDaemon();
    message_listener_->connectDaemon();

    // Sleep 500 millseconds to wait seafile registering services

    msleep(500);

    //
    // load proxy settings (important)
    //
    settings_mgr_->loadSettings();

    //
    // start network-related services
    //
    NetworkStatusDetector::instance()->start();
    FileCacheDB::instance()->start();
    AutoUpdateManager::instance()->start();

    AvatarService::instance()->start();
    ThumbnailService::instance()->start();

    SeahubNotificationsMonitor::instance()->start();
    ServerStatusService::instance()->start();
    CustomizationService::instance()->start();
    AccountInfoService::instance()->start();

    account_mgr_->updateServerInfo();

    //
    // start ui part
    //
    main_win_ = new MainWindow;

#if defined(Q_OS_MAC)
    seafApplet->settingsManager()->setHideDockIcon(seafApplet->settingsManager()->hideDockIcon());
#endif

    if (configurator_->firstUse() || account_mgr_->accounts().size() == 0) {
        do {
            QString username = readPreconfigureExpandedString(kPreconfigureUsername);
            QString token = readPreconfigureExpandedString(kPreconfigureUserToken);
            QString url = readPreconfigureExpandedString(kPreconfigureServerAddr);
            QString computer_name = readPreconfigureExpandedString(kPreconfigureComputerName, settingsManager()->getComputerName());
            if (!computer_name.isEmpty())
                settingsManager()->setComputerName(computer_name);
            if (!username.isEmpty() && !token.isEmpty() && !url.isEmpty()) {
                Account account(url, username, token);
                if (account_mgr_->saveAccount(account) < 0) {
                    warningBox(tr("failed to add default account"));
                    exit(1);
                }
                break;
            }

            if (readPreconfigureEntry(kHideConfigurationWizard).toInt())
                break;
            LoginDialog login_dialog;
            login_dialog.exec();
        } while (0);
    }

    started_ = true;

    if (configurator_->firstUse() || !settings_mgr_->hideMainWindowWhenStarted()) {
        main_win_->showWindow();
    }

    tray_icon_->start();
    tray_icon_->setState(SeafileTrayIcon::STATE_DAEMON_UP);

#if defined(Q_OS_WIN32)
    QTimer::singleShot(kIntervalBeforeShowInitVirtualDialog, this, SLOT(checkInitVDrive()));
    configurator_->installCustomUrlHandler();
#endif

    QString value;
    if (seafApplet->rpcClient()->seafileGetConfig("client_name", &value) < 0 || value.isEmpty()) {
        // We do this because clients before 6.0 don't set the "client_name" option.
        seafApplet->rpcClient()->seafileSetConfig(
        "client_name", settings_mgr_->getComputerName());
    }

    OpenLocalHelper::instance()->checkPendingOpenLocalRequest();

    QTimer::singleShot(kIntervalForUpdateRepoProperty,
                       this, SLOT(updateReposPropertyForHttpSync()));

    //
    // start finder/explorer extension handler
    //
#if defined(Q_OS_WIN32)
    SeafileExtensionHandler::instance()->start();
#elif defined(HAVE_FINDER_SYNC_SUPPORT)
    finderSyncListenerStart();
#endif

#ifdef HAVE_SPARKLE_SUPPORT
    if (AutoUpdateService::instance()->shouldSupportAutoUpdate()) {
        AutoUpdateService::instance()->start();
    }
#endif
}

void SeafileApplet::checkInitVDrive()
{
    if (configurator_->firstUse() && account_mgr_->hasAccount()) {
        const Account account = account_mgr_->currentAccount();
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

void SeafileApplet::onAboutToQuit()
{
    tray_icon_->hide();
    if (main_win_) {
        main_win_->writeSettings();
    }
}
// stop the main event loop and return to the main function
void SeafileApplet::errorAndExit(const QString& error)
{
    if (in_exit_ || QCoreApplication::closingDown()) {
        return;
    }

    in_exit_ = true;

    warningBox(error);
    // stop eventloop before exit and return to the main function
    QCoreApplication::exit(1);
}

void SeafileApplet::restartApp()
{
    if (in_exit_ || QCoreApplication::closingDown()) {
        return;
    }

    in_exit_ = true;

    QStringList args = QApplication::arguments();

    args.removeFirst();

    // append delay argument
    bool found = false;
    Q_FOREACH(const QString& arg, args)
    {
        if (arg == "--delay" || arg == "-D") {
            found = true;
            break;
        }
    }

    if (!found)
        args.push_back("--delay");

    QProcess::startDetached(QApplication::applicationFilePath(), args);
    QCoreApplication::quit();
}

void SeafileApplet::initLog()
{
    if (applet_log_init(toCStr(configurator_->ccnetDir())) < 0) {
        errorAndExit(tr("Failed to initialize log: %s").arg(g_strerror(errno)));
    } else {
        // give a change to override DEBUG_LEVEL by environment
        QString debug_level = qgetenv("SEAFILE_CLIENT_DEBUG");
        if (!debug_level.isEmpty() && debug_level != "false" &&
            debug_level != "0")
            seafile_client_debug_level = DEBUG;

        if (seafile_client_debug_level == DEBUG)
            qInstallMessageHandler(myLogHandlerDebug);
        else
            qInstallMessageHandler(myLogHandler);
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

#if defined(Q_OS_WIN32)
    loadQss("qt-win.css") || loadQss(":/qt-win.css");
#elif defined(Q_OS_LINUX)
    loadQss("qt-linux.css") || loadQss(":/qt-linux.css");
#else
    loadQss("qt-mac.css") || loadQss(":/qt-mac.css");
#endif
}

void SeafileApplet::warningBox(const QString& msg, QWidget *parent)
{
    QMessageBox box(parent ? parent : main_win_);
    box.setText(msg);
    box.setWindowTitle(getBrand());
    box.setIcon(QMessageBox::Warning);
    box.addButton(tr("OK"), QMessageBox::YesRole);
    box.exec();

    if (!parent) {
        main_win_->showWindow();
    }
    qWarning("%s", msg.toUtf8().data());
}

void SeafileApplet::messageBox(const QString& msg, QWidget *parent)
{
    QMessageBox box(parent ? parent : main_win_);
    box.setText(msg);
    box.setWindowTitle(getBrand());
    box.setIcon(QMessageBox::Information);
    box.addButton(tr("OK"), QMessageBox::YesRole);
    box.exec();
    qDebug("%s", msg.toUtf8().data());
}

bool SeafileApplet::yesOrNoBox(const QString& msg, QWidget *parent, bool default_val)
{
    QMessageBox box(parent ? parent : main_win_);
    box.setText(msg);
    box.setWindowTitle(getBrand());
    box.setIcon(QMessageBox::Question);
    QPushButton *yes_btn = box.addButton(tr("Yes"), QMessageBox::YesRole);
    QPushButton *no_btn = box.addButton(tr("No"), QMessageBox::NoRole);
    box.setDefaultButton(default_val ? yes_btn: no_btn);
    box.exec();

    return box.clickedButton() == yes_btn;
}

bool SeafileApplet::yesOrCancelBox(const QString& msg, QWidget *parent, bool default_yes)
{
    QMessageBox box(parent ? parent : main_win_);
    box.setText(msg);
    box.setWindowTitle(getBrand());
    box.setIcon(QMessageBox::Question);
    QPushButton *yes_btn = box.addButton(tr("Yes"), QMessageBox::YesRole);
    QPushButton *cancel_btn = box.addButton(tr("Cancel"), QMessageBox::RejectRole);
    box.setDefaultButton(default_yes ? yes_btn: cancel_btn);
    box.exec();

    return box.clickedButton() == yes_btn;
}


QMessageBox::StandardButton
SeafileApplet::yesNoCancelBox(const QString& msg, QWidget *parent, QMessageBox::StandardButton default_btn)
{
    QMessageBox box(parent ? parent : main_win_);
    box.setText(msg);
    box.setWindowTitle(getBrand());
    box.setIcon(QMessageBox::Question);
    QPushButton *yes_btn = box.addButton(tr("Yes"), QMessageBox::YesRole);
    QPushButton *no_btn = box.addButton(tr("No"), QMessageBox::NoRole);
    box.addButton(tr("Cancel"), QMessageBox::RejectRole);
    box.setDefaultButton(default_btn);
    box.exec();

    QAbstractButton *btn = box.clickedButton();
    if (btn == yes_btn) {
        return QMessageBox::Yes;
    } else if (btn == no_btn) {
        return QMessageBox::No;
    }

    return QMessageBox::Cancel;
}

bool SeafileApplet::detailedYesOrNoBox(const QString& msg, const QString& detailed_text, QWidget *parent, bool default_val)
{
    QMessageBox msgBox(QMessageBox::Question,
                       getBrand(),
                       msg,
                       QMessageBox::Yes | QMessageBox::No,
                       parent != 0 ? parent : main_win_);
    msgBox.setDetailedText(detailed_text);
    msgBox.setButtonText(QMessageBox::Yes, tr("Yes"));
    msgBox.setButtonText(QMessageBox::No, tr("No"));
    // Turns out the layout box in the QMessageBox is a grid
    // You can force the resize using a spacer this way:
    QSpacerItem* horizontalSpacer = new QSpacerItem(400, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
    QGridLayout* layout = (QGridLayout*)msgBox.layout();
    layout->addItem(horizontalSpacer, layout->rowCount(), 0, 1, layout->columnCount());
    msgBox.setDefaultButton(default_val ? QMessageBox::Yes : QMessageBox::No);
    return msgBox.exec() == QMessageBox::Yes;
}

/**
 * For each repo, add the "server-url" property (inferred from account url),
 * which would be used for http sync.
 */
void SeafileApplet::updateReposPropertyForHttpSync()
{
    std::vector<LocalRepo> repos;
    if (rpc_client_->listLocalRepos(&repos) < 0) {
        QTimer::singleShot(kIntervalForUpdateRepoProperty,
                           this, SLOT(updateReposPropertyForHttpSync()));
        return;
    }

    const std::vector<Account>& accounts = account_mgr_->accounts();
    for (size_t i = 0; i < repos.size(); i++) {
        const LocalRepo& repo = repos[i];
        QString repo_server_url;
        QString relay_addr;
        if (rpc_client_->getRepoProperty(repo.id, kRepoServerUrlProperty, &repo_server_url) < 0) {
            continue;
        }
        if (!repo_server_url.isEmpty()) {
            continue;
        }
        if (rpc_client_->getRepoProperty(repo.id, kRepoRelayAddrProperty, &relay_addr) < 0) {
            continue;
        }
        for (size_t i = 0; i < accounts.size(); i++) {
            const Account& account = accounts[i];
            if (account.serverUrl.host() == relay_addr) {
                QUrl url(account.serverUrl);
                url.setPath("/");
                rpc_client_->setRepoProperty(repo.id, kRepoServerUrlProperty, url.toString());
                break;
            }
        }
    }
}

QVariant SeafileApplet::readPreconfigureEntry(const QString& key, const QVariant& default_value)
{
#ifdef Q_OS_WIN32
    QVariant v = RegElement::getPreconfigureValue(key);
    if (!v.isNull()) {
        return v;
    }
#endif
    QString configure_file = QDir::home().filePath(kSeafileConfigureFileName);
    if (!QFileInfo(configure_file).exists())
        return default_value;
    QSettings setting(configure_file, QSettings::IniFormat);
    setting.beginGroup(kSeafilePreconfigureGroupName);
    QVariant value = setting.value(key, default_value);
    setting.endGroup();
    return value;
}

QString SeafileApplet::readPreconfigureExpandedString(const QString& key, const QString& default_value)
{
    QVariant retval = readPreconfigureEntry(key, default_value);
    if (retval.isNull() || retval.type() != QVariant::String)
        return QString();
    return expandVars(retval.toString());
}
