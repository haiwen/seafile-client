#include <QApplication>
#include <glib-object.h>

#include "seafile-applet.h"
#include "QtAwesome.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    g_type_init();

    awesome = new QtAwesome(qApp);
    awesome->initFontAwesome();

    seafApplet = new SeafileApplet;
    seafApplet->start();

    return app.exec();
}
