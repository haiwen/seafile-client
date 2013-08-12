#include <QApplication>

#include "ui/main-window.h"
#include "rpc/rpc-client.h"
#include "seafile-applet.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    g_type_init();

    seafApplet = new SeafileApplet();
    seafApplet->start();

    MainWindow mainWin;
    mainWin.show();

    return app.exec();
}
