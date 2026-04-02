#include <QApplication>
#include <QCommandLineParser>
#include <QMessageBox>
#include <QWidget>
#include <QDir>

#include <glib-object.h>
#include <cstdio>

#include "i18n.h"
#include "crash-handler.h"
#include "utils/utils.h"
#include "utils/paint-utils.h"
#include "utils/process.h"
#include "utils/uninstall-helpers.h"
#include "ui/proxy-style.h"
#include "seafile-applet.h"
#include "QtAwesome.h"
#include "open-local-helper.h"
#if defined(Q_OS_WIN32)
#include "utils/utils-win.h"
#endif
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

#if defined(Q_OS_WIN32)
void initBreakpad()
{
#ifdef SEAFILE_CLIENT_HAS_CRASH_REPORTER
    // if we have built with breakpad, load it in run time
    Breakpad::CrashHandler::instance()->Init(
        QDir(defaultCcnetDir()).absoluteFilePath("crash-applet"));
#endif
}
#endif

void setupHIDPIFix()
{
    // enable builtin retina mode
    // http://blog.qt.digia.com/blog/2013/04/25/retina-display-support-for-mac-os-ios-and-x11/
    // https://qt.gitorious.org/qt/qtbase/source/a3cb057c3d5c9ed2c12fb7542065c3d667be38b7:src/gui/image/qicon.cpp#L1028-1043
    qApp->setAttribute(Qt::AA_UseHighDpiPixmaps);

  #if defined(Q_OS_WIN32)
    if (!utils::win::fixQtHDPINonIntegerScaling()) {
        qApp->setAttribute(Qt::AA_EnableHighDpiScaling);
    }
  #elif !defined(Q_OS_MAC)
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

void handleCommandLineOption(const QApplication &app)
{
    QCommandLineParser parser;

    parser.addOptions({
        QCommandLineOption(QStringList({"D", "delay"})),
        QCommandLineOption(QStringList({"K", "stop"})),
        QCommandLineOption(QStringList({"P", "ping"})),
        QCommandLineOption(QStringList({"X", "remove-user-data"})),
        QCommandLineOption(QStringList({"c", "config-dir"}), "The config dir", "config"),
        QCommandLineOption(QStringList({"d", "data-dir"}), "The data dir", "data"),
        QCommandLineOption(QStringList({"f", "open-local-file"}), "Open a local file", "file"),
        QCommandLineOption(QStringList({"l", "stdout"}))
    });
    parser.process(app);

    if (parser.isSet("D")) {
        msleep(1000);
    }
    if (parser.isSet("c")) {
        QByteArray path = parser.value("c").toUtf8();
        g_setenv("CCNET_CONF_DIR", path.constData(), 1);
    }
    if (parser.isSet("d")) {
        QByteArray path = parser.value("d").toUtf8();
        g_setenv("SEAFILE_DATA_DIR", path.constData(), 1);
    }
    if (parser.isSet("l")) {
        g_setenv("LOG_STDOUT", "", 1);
    }
    if (parser.isSet("K")) {
        do_stop();
        exit(0);
    }
    if (parser.isSet("P")) {
        do_ping();
        exit(0);
    }
    if (parser.isSet("X")) {
        do_remove_user_data();
        exit(0);
    }
    if (parser.isSet("f")) {
        OpenLocalHelper::instance()->handleOpenLocalFromCommandLine(parser.value("f"));
    }
}

} // anonymous namespace

int main(int argc, char *argv[])
{
    int ret = 0;

    // call glib's init functions
    initGlib();

    // initialize breakpad if enabled
#if defined(Q_OS_WIN32)
    initBreakpad();
#endif

    // Apply hidpi support
    setupHIDPIFix();

#if defined(Q_OS_WIN32)
    // When the user start seafile applet from the windows "Start" menu, the
    // working directory is set to the parent folder of the seafile-applet.exe.
    // See https://github.com/haiwen/seafile/blob/v6.0.1/msi/seafile.wxs#L60
    //
    // Sometimes the seafile-applet program would abort with error messgages
    // "can't find qt plugin windows dll". So here we add current directory to
    // the library path, hopefully fixing that problem.
    QCoreApplication::addLibraryPath(".\\");
#endif

    // TODO imple if we have to restart the application
    // the manual at http://qt-project.org/wiki/ApplicationRestart
#if defined(Q_OS_MAC)
    Application app(argc, argv);
#else
    QApplication app(argc, argv);
#endif

    // don't quit even if the last windows is closed
    app.setQuitOnLastWindowClosed(false);

    // set the domains of settings
    setupSettingDomain();

    // initialize i18n settings
    I18NHelper::getInstance()->init();

    // initialize style settings
    app.setStyle(new SeafileProxyStyle());

    // start applet
    SeafileApplet mApplet;
    seafApplet = &mApplet;

    // handle with the command arguments
    handleCommandLineOption(app);

    // count if we have any instance running now. if more than one, exit
    if (count_process(APPNAME) > 1) {
        if (OpenLocalHelper::instance()->activateRunningInstance()) {
            printf("Activated running instance of seafile client\n");
            return 0;
        }
        QMessageBox::warning(NULL, getBrand(),
                             QObject::tr("%1 Client is already running").arg(getBrand()),
                             QMessageBox::Ok);
        return -1;
    }

    // init qtawesome component
    awesome = new QtAwesome(qApp);
    awesome->initFontAwesome();

    seafApplet->start();

    // qWarning("globalDevicePixelRatio() = %f\n", globalDevicePixelRatio());
    // printf("globalDevicePixelRatio() = %f\n", globalDevicePixelRatio());
    // fflush(stdout);

    // start qt eventloop
    ret = app.exec();

    return ret;
}
