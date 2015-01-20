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
#if defined(Q_OS_MAC)
#include "application.h"
#endif

#define APPNAME "seafile-applet"

int main(int argc, char *argv[])
{
    int ret = 0;
    char c;
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

#if !GLIB_CHECK_VERSION(2, 35, 0)
    g_type_init();
#endif
#if !GLIB_CHECK_VERSION(2, 31, 0)
    g_thread_init(NULL);
#endif

#if defined(Q_OS_MAC)
    Application app(argc, argv);
#else
    QApplication app(argc, argv);
#endif

    // enable builtin retina mode for MAC
    // http://blog.qt.digia.com/blog/2013/04/25/retina-display-support-for-mac-os-ios-and-x11/
    // https://qt.gitorious.org/qt/qtbase/source/a3cb057c3d5c9ed2c12fb7542065c3d667be38b7:src/gui/image/qicon.cpp#L1028-1043
#if QT_VERSION >= QT_VERSION_CHECK(5, 1, 0) && defined(Q_OS_MAC)
    app.setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

    QDir::setCurrent(QApplication::applicationDirPath());

#ifdef SEAFILE_CLIENT_HAS_CRASH_REPORTER
    // if we have built with breakpad, load it in run time
    Breakpad::CrashHandler::instance()->Init(
        QDir(defaultCcnetDir()).absoluteFilePath("crash-applet"));
#endif

    // see QSettings documentation
    QCoreApplication::setOrganizationName(getBrand());
    QCoreApplication::setOrganizationDomain("seafile.com");
    QCoreApplication::setApplicationName(QString("%1 Client").arg(getBrand()));

    // initialize i18n
    I18NHelper::getInstance()->init();

    app.setStyle(new SeafileProxyStyle());

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

    if (count_process(APPNAME) > 1) {
        QMessageBox::warning(NULL, getBrand(),
                             QObject::tr("%1 is already running").arg(getBrand()),
                             QMessageBox::Ok);
        return -1;
    }

    app.setQuitOnLastWindowClosed(false);

    awesome = new QtAwesome(qApp);
    awesome->initFontAwesome();

    {
        SeafileApplet mApplet;
        seafApplet = &mApplet;
        seafApplet->start();
        ret = app.exec();
        //destroy SeafileApplet instance after QEventLoop returns
    }

    return ret;
}
