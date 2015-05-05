#include "application.h"

#include <objc/objc.h>
#include <objc/message.h>

#include <QFileOpenEvent>

#include "open-local-helper.h"
#include "ui/main-window.h"
#include "seafile-applet.h"
static bool dockClickHandler(id self,SEL _cmd,...)
{
    Q_UNUSED(self)
    Q_UNUSED(_cmd)
    if (seafApplet) {
        MainWindow *main_win = seafApplet->mainWindow();
        if (main_win)
            main_win->showWindow();
    }
    return true;
}

Application::Application (int &argc, char **argv):QApplication(argc, argv)
{
    objc_object* cls = (objc_object *)objc_getClass("NSApplication");
    SEL sharedApplication = sel_registerName("sharedApplication");
    objc_object* appInst = objc_msgSend(cls,sharedApplication);

    if(appInst != NULL)
    {
        objc_object* delegate = objc_msgSend(appInst, sel_registerName("delegate"));
        objc_object* delClass = objc_msgSend(delegate,  sel_registerName("class"));
        class_addMethod((Class)delClass, sel_registerName("applicationShouldHandleReopen:hasVisibleWindows:"), (IMP)dockClickHandler, "B@:");
    }
}

bool Application::event(QEvent *e)
{
    if (e->type() == QEvent::FileOpen)
    {
        QFileOpenEvent *event = static_cast<QFileOpenEvent *>(e);
        // this event has native mac handler callee
        if(event && event->url().scheme() == "seafile")
        {
            qWarning("[FileOpen] trying to open %s\n", event->url().toEncoded().data());
            if (!seafApplet->started())
                OpenLocalHelper::instance()->setUrl(event->url().toEncoded().data());
            else
                OpenLocalHelper::instance()->openLocalFile(event->url());
            return true;
        }
    }
    return QApplication::event(e);
}
