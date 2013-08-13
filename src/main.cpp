#include <QApplication>
#include <glib-object.h>

#include "seafile-applet.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    g_type_init();

    seafApplet = new SeafileApplet;
    seafApplet->start();

    return app.exec();
}
