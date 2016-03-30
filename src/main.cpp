#include <getopt.h>
#include <QApplication>
#include <QMessageBox>
#include <QWidget>
#include <QDir>

#include <glib-object.h>
#include <cstdio>

#include "i18n.h"
#include "crash-handler.h"
#include "utils/utils.h"
#include "utils/process.h"
#include "utils/uninstall-helpers.h"
#include "shared-application.h"
#include "ui/proxy-style.h"
#include "seafile-applet.h"
#include "QtAwesome.h"
#include "open-local-helper.h"
#if defined(Q_OS_MAC)
#include "application.h"
#endif

#define APPNAME "seafile-applet"

namespace {
void initGlib()
{
#if !GLIB_CHECK_VERSION(2, 35, 0)
    g_type_init();
#endif
#if !GLIB_CHECK_VERSION(2, 31, 0)
    g_thread_init(NULL);
#endif
}

void initBreakpad()
{
#ifdef SEAFILE_CLIENT_HAS_CRASH_REPORTER
    // if we have built with breakpad, load it in run time
    Breakpad::CrashHandler::instance()->Init(
        QDir(defaultCcnetDir()).absoluteFilePath("crash-applet"));
#endif
}

void setupFontFix()
{
#if QT_VERSION < QT_VERSION_CHECK(4, 8, 6) && defined(Q_OS_MAC)
    // Mac OS X 10.9 (mavericks) font issue,
    // fixed in qt4.8.6 and qt5.x
    // https://bugreports.qt-project.org/browse/QTBUG-32789
    if ( QSysInfo::MacintoshVersion > QSysInfo::MV_10_8 ) {
        QFont::insertSubstitution(".Lucida Grande UI", "Lucida Grande");
    }
#endif // QT_VERSION_CHECK(4, 8, 6)

#if QT_VERSION < QT_VERSION_CHECK(5, 3, 2) && defined(Q_OS_MAC)
    // Text in buttons and drop-downs looks misaligned in osx 10.10,
    // fixed in qt5.3.2
    // https://bugreports.qt-project.org/browse/QTBUG-40833
    if ( QSysInfo::MacintoshVersion > QSysInfo::MV_10_8 ) {
        QFont::insertSubstitution(".Helvetica Neue DeskInterface", "Helvetica Neue");
    }
#endif // QT_VERSION_CHECK(5, 3, 2)
}

void setupHIDPIFix()
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 1, 0)
    // enable builtin retina mode
    // http://blog.qt.digia.com/blog/2013/04/25/retina-display-support-for-mac-os-ios-and-x11/
    // https://qt.gitorious.org/qt/qtbase/source/a3cb057c3d5c9ed2c12fb7542065c3d667be38b7:src/gui/image/qicon.cpp#L1028-1043
    qApp->setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0) && !defined(Q_OS_MAC)
    // Enable HDPI auto detection.
    // See http://blog.qt.io/blog/2016/01/26/high-dpi-support-in-qt-5-6/
    qApp->setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
}

void setupSettingDomain()
{
    // see QSettings documentation
    QCoreApplication::setOrganizationName(getBrand());
    QCoreApplication::setOrganizationDomain("seafile.com");
    QCoreApplication::setApplicationName(QString("%1 Client").arg(getBrand()));
}

void handleCommandLineOption(int argc, char *argv[])
{
    int c;
    static const char *short_options = "KDXc:d:f:";
    static const struct option long_options[] = {
        { "config-dir", required_argument, NULL, 'c' },
        { "data-dir", required_argument, NULL, 'd' },
        { "stop", no_argument, NULL, 'K' },
        { "delay", no_argument, NULL, 'D' },
        { "remove-user-data", no_argument, NULL, 'X' },
        { "open-local-file", no_argument, NULL, 'f' },
        { "stdout", no_argument, NULL, 'l' },
        { NULL, 0, NULL, 0, },
    };

    while ((c = getopt_long (argc, argv, short_options,
                             long_options, NULL)) != EOF) {
        switch (c) {
        case 'c':
            g_setenv ("CCNET_CONF_DIR", optarg, 1);
            break;
        case 'd':
            g_setenv ("SEAFILE_DATA_DIR", optarg, 1);
            break;
        case 'l':
            g_setenv ("LOG_STDOUT", "", 1);
            break;
        case 'K':
            do_stop();
            exit(0);
        case 'D':
            msleep(1000);
            break;
        case 'X':
            do_remove_user_data();
            exit(0);
        case 'f':
            OpenLocalHelper::instance()->handleOpenLocalFromCommandLine(optarg);
            break;
        default:
            exit(1);
        }
    }

}

} // anonymous namespace

int main(int argc, char *argv[])
{
    int ret = 0;

    // call glib's init functions
    initGlib();

    // initialize breakpad if enabled
    initBreakpad();

    // Apply hidpi support
    setupHIDPIFix();

    // TODO imple if we have to restart the application
    // the manual at http://qt-project.org/wiki/ApplicationRestart
#if defined(Q_OS_MAC)
    Application app(argc, argv);
#else
    QApplication app(argc, argv);
#endif
#if defined(Q_OS_WIN32)
    // change the current directory
    QDir::setCurrent(QApplication::applicationDirPath());
#endif

    // don't quit even if the last windows is closed
    app.setQuitOnLastWindowClosed(false);

    // apply some ui fixes for mac
    setupFontFix();

    // set the domains of settings
    setupSettingDomain();

    // initialize i18n settings
    I18NHelper::getInstance()->init();

    // initialize style settings
    app.setStyle(new SeafileProxyStyle());

    // handle with the command arguments
    handleCommandLineOption(argc, argv);

    // count if we have any instance running now. if more than one, exit
    if (count_process(APPNAME) > 1) {
        // have we activated it ? exit
        if (SharedApplication::activate())
            return 0;
        if (QMessageBox::No == QMessageBox::warning(NULL, getBrand(),
                QObject::tr("Found another running process of %1, kill it and start a new one?").arg(getBrand()),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes)) {
            return -1;
        }

        // sleep 9 * 100ms to await the os completing the operation
        do_stop();
        int n = 10;
        while(--n >0 && count_process(APPNAME) > 1)
            msleep(100);

        // force shutdown it
        if (count_process(APPNAME) > 1) {
            shutdown_process(APPNAME);
            msleep(100);
        }

        // count if we still have any instance running now. if more than one, exit
        if (count_process(APPNAME) > 1) {
            QMessageBox::critical(NULL, getBrand(),
                QObject::tr("Unable to start %1 due to the failure of shutting down the previous process").arg(getBrand()),
                QMessageBox::Ok);
            return -1;
        }
    }

    // init qtawesome component
    awesome = new QtAwesome(qApp);
    awesome->initFontAwesome();

    // start applet
    SeafileApplet mApplet;
    seafApplet = &mApplet;
    seafApplet->start();

    // start qt eventloop
    ret = app.exec();

    return ret;
}
