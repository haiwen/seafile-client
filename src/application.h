/**
 * Show thr main window when the dock icon is clicked
 */
#include <QApplication>

#include "ui/main-window.h"
class Application : public QApplication {
    Q_OBJECT

public:

    Application (int& argc, char **argv);
    virtual ~Application() {};
};
