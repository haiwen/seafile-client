#include "application.h"
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

//TODO use modern Objc (Objc 2.0) to replace these deprecated APIs
Application::Application (int &argc, char **argv):QApplication(argc, argv)
{
    objc_object* cls = (objc_object *)objc_getClass("NSApplication");
    SEL sharedApplication = sel_registerName("sharedApplication");
    objc_object* appInst = objc_msgSend(cls,sharedApplication);

    if(appInst != NULL)
    {
        objc_object* delegate = objc_msgSend(appInst, sel_registerName("delegate"));
        objc_object* delClass = objc_msgSend(delegate,  sel_registerName("class"));
        class_addMethod((objc_class*)delClass, sel_registerName("applicationShouldHandleReopen:hasVisibleWindows:"), (IMP)dockClickHandler,"B@:");
    }
}
