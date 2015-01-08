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
#include "ui/proxy-style.h"
#include "seafile-applet.h"
#include "QtAwesome.h"
#include "open-local-helper.h"
#if defined(Q_WS_MAC)
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
#if defined(Q_WS_MAC)
    if (QSysInfo::MacintoshVersion > QSysInfo::MV_10_8) {
        // fix Mac OS X 10.9 (mavericks) font issue
        // https://bugreports.qt-project.org/browse/QTBUG-32789
        QFont::insertSubstitution(".Lucida Grande UI", "Lucida Grande");
        // https://bugreports.qt-project.org/browse/QTBUG-40833
        QFont::insertSubstitution(".Helvetica Neue DeskInterface", "Helvetica Neue");
    }
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
    char c;
    static const char *short_options = "KXc:d:f:";
    static const struct option long_options[] = {
        { "config-dir", required_argument, NULL, 'c' },
        { "data-dir", required_argument, NULL, 'd' },
        { "stop", no_argument, NULL, 'K' },
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

    // TODO imple if we have to restart the application
    // the manual at http://qt-project.org/wiki/ApplicationRestart
#if defined(Q_WS_MAC)
    Application app(argc, argv);
#else
    QApplication app(argc, argv);
#endif
    // change the current directory
    QDir::setCurrent(QApplication::applicationDirPath());

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
        QMessageBox::warning(NULL, getBrand(),
                             QObject::tr("%1 is already running").arg(getBrand()),
                             QMessageBox::Ok);
        return -1;
    }

    // init qtawesome component
    // we are leaking memory here, (with QFontDatabase)
    // it is strange,but i have no way to fix currently
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
