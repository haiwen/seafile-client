#include <getopt.h>
#include <QApplication>
#include <QMessageBox>
#include <QTranslator>
#include <QLocale>
#include <QLibraryInfo>
#include <QWidget>
#include <QDir>

#include <glib-object.h>
#include <stdio.h>

#include "utils/process.h"
#include "utils/uninstall-helpers.h"
#include "ui/proxy-style.h"
#include "seafile-applet.h"
#include "QtAwesome.h"
#include "open-local-helper.h"
#ifdef Q_WS_MAC
#include "Application.h"
#endif

#define APPNAME "seafile-applet"

namespace {


} // namespace


int main(int argc, char *argv[])
{
#ifdef Q_WS_MAC
    if ( QSysInfo::MacintoshVersion > QSysInfo::MV_10_8 ) {
        // fix Mac OS X 10.9 (mavericks) font issue
        // https://bugreports.qt-project.org/browse/QTBUG-32789
        QFont::insertSubstitution(".Lucida Grande UI", "Lucida Grande");
    }
#endif

#if !GLIB_CHECK_VERSION(2, 35, 0)
    g_type_init();
#endif
#if !GLIB_CHECK_VERSION(2, 31, 0)
    g_thread_init(NULL);
#endif

#ifdef Q_WS_MAC
    Application app(argc, argv);
#else
    QApplication app(argc, argv);
#endif

    QDir::setCurrent(QApplication::applicationDirPath());
    app.setStyle(new SeafileProxyStyle());

    // initialize i18n
    QTranslator qtTranslator;
#if defined(Q_WS_WIN)
    qtTranslator.load("qt_" + QLocale::system().name());
#else
    qtTranslator.load("qt_" + QLocale::system().name(),
                      QLibraryInfo::location(QLibraryInfo::TranslationsPath));
#endif
    app.installTranslator(&qtTranslator);

    QTranslator myappTranslator;
#if QT_VERSION >= 0x040800 && not defined(Q_WS_MAC)
    myappTranslator.load(QLocale::system(), // locale
                         "",                // file name
                         "seafile_",        // prefix
                         ":/i18n/",         // folder
                         ".qm");            // suffix
#else
    myappTranslator.load(QString(":/i18n/seafile_%1.qm").arg(QLocale::system().name()));
#endif

    app.installTranslator(&myappTranslator);

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

    char c;
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

    // see QSettings documentation
    QCoreApplication::setOrganizationName(getBrand());
    QCoreApplication::setOrganizationDomain("seafile.com");
    QCoreApplication::setApplicationName(QString("%1 Client").arg(getBrand()));

    awesome = new QtAwesome(qApp);
    awesome->initFontAwesome();

    seafApplet = new SeafileApplet;
    seafApplet->start();

    return app.exec();
}
