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
#include "seafile-applet.h"
#include "QtAwesome.h"
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

    static const char *short_options = "KXc:";
    static const struct option long_options[] = {
        { "config-dir", required_argument, NULL, 'c' },
        { "stop", no_argument, NULL, 'K' },
        { "remove-user-data", no_argument, NULL, 'X' },
        { NULL, 0, NULL, 0, },
    };

    char c;
    while ((c = getopt_long (argc, argv, short_options,
                             long_options, NULL)) != EOF) {
        switch (c) {
        case 'c':
            g_setenv ("CCNET_CONF_DIR", optarg, 1);
            break;
        case 'K':
            do_stop();
            exit(0);
        case 'X':
            do_remove_user_data();
            exit(0);
        default:
            exit(1);
        }
    }

#ifdef Q_WS_MAC
    Application app(argc, argv);
#else
    QApplication app(argc, argv);
#endif

    if (count_process(APPNAME) > 1) {
        QMessageBox::warning(NULL, SEAFILE_CLIENT_BRAND,
                             QObject::tr("%1 is already running").arg(SEAFILE_CLIENT_BRAND),
                             QMessageBox::Ok);
        return -1;
    }

    QDir::setCurrent(QApplication::applicationDirPath());

    app.setQuitOnLastWindowClosed(false);

    // see QSettings documentation
    QCoreApplication::setOrganizationName(SEAFILE_CLIENT_BRAND);
    QCoreApplication::setOrganizationDomain("seafile.com");
    QCoreApplication::setApplicationName(QString("%1 Client").arg(SEAFILE_CLIENT_BRAND));

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
    myappTranslator.load(QString(":/i18n/seafile_%1.qm").arg(QLocale::system().name()));
    app.installTranslator(&myappTranslator);

    awesome = new QtAwesome(qApp);
    awesome->initFontAwesome();

    seafApplet = new SeafileApplet;
    seafApplet->start();

    return app.exec();
}
