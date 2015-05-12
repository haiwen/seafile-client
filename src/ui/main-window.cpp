#include <QtGlobal>

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
#include <QtWidgets>
#else
#include <QtGui>
#endif
#include <QApplication>
#include <QDesktopServices>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QCoreApplication>

#include <QDialog>
#include <QTabBar>
#include <QVBoxLayout>

#include "cloud-view.h"
#include "utils/utils.h"
#include "seafile-applet.h"
#include "configurator.h"
#include "tray-icon.h"
#include "login-dialog.h"
#include "utils/utils.h"
#include "utils/utils-mac.h"

#include "main-window.h"

namespace {

enum WIDGET_INDEX {
    INDEX_CLOUD_VIEW = 0,
    INDEX_LOCAL_VIEW
};

/*
void showTestDialog(QWidget *parent)
{
    static QDialog *dialog;

    if (!dialog) {
        dialog = new QDialog(parent);

        QVBoxLayout *layout = new QVBoxLayout;
        dialog->setLayout(layout);

        QTabBar *bar = new QTabBar;
        bar->setExpanding(true);
        bar->addTab("TabA");
        bar->addTab("TabB");
        bar->addTab("TabC");

        layout->addWidget(bar);
    }

    // dialog.exec();
    dialog->show();
    dialog->raise();
    dialog->activateWindow();
}
*/


} // namespace

/// a little helper function to detect if the pos is outside the screens
static bool isOutsideScreens(const QRect &rect) {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    QList<QScreen*> screens = QGuiApplication::screens();
    for (int i = 0; i < screens.size(); ++i) {
        if (screens[i]->geometry().contains(rect))
            return false;
    }
#else
    QDesktopWidget *desktop = QApplication::desktop();
    for (int i = 0; i < desktop->numScreens(); ++i) {
        if (desktop->screenGeometry(i).contains(rect))
            return false;
    }
#endif
    return true;
}


MainWindow::MainWindow()
{
    setWindowIcon(QIcon(":/images/seafile.png"));
    setWindowTitle(getBrand());

    // Qt::Tool hides the taskbar entry on windows
    // setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint);

    setWindowFlags(Qt::Window
#if !defined(Q_OS_MAC)
                   | Qt::FramelessWindowHint
#endif
#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
                   | Qt::WindowMinimizeButtonHint
#endif
                   | Qt::WindowSystemMenuHint);

    cloud_view_ = new CloudView;

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(cloud_view_);

    QWidget *wrapper = new QWidget;
    wrapper->setObjectName("mainWrapper");
    wrapper->setLayout(layout);

    setCentralWidget(wrapper);

    createActions();
    setAttribute(Qt::WA_TranslucentBackground, true);

#if defined(Q_OS_MAC) && (QT_VERSION >= QT_VERSION_CHECK(5, 2, 0))
    connect(qApp, SIGNAL(applicationStateChanged(Qt::ApplicationState)),
            this, SLOT(checkShowWindow()));
#endif
}

void MainWindow::hide()
{
    writeSettings();
    QMainWindow::hide();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    hide();
    // event->ignore();
}

bool MainWindow::event(QEvent *ev)
{
    bool ret = QMainWindow::event(ev);

    if (isMinimized() && ev->type() == QEvent::WindowStateChange) {
        QWindowStateChangeEvent *wev = (QWindowStateChangeEvent *)ev;
        if (wev->oldState() != Qt::WindowMinimized) {
            writeSettings();
        }
    }

    if (ev->type() == QEvent::Hide) {
        writeSettings();
    }

    return ret;
}

void MainWindow::changeEvent(QEvent *event)
{
// #if defined(Q_OS_WIN32)
//     /*
//      * Solve the problem of restoring a minimized frameless window on Windows
//      * See http://stackoverflow.com/questions/18614661/how-to-not-hide-taskbar-item-during-using-hide
//      */
//     if(event->type() == QEvent::WindowStateChange) {
//         if(windowState() & Qt::WindowMinimized ) {
//             //do something after minimize
//         } else {
//             cloud_view_->hide();
//             cloud_view_->show();
//         }
//     }
// #endif
}

void MainWindow::showEvent(QShowEvent *event)
{
    readSettings();
#if defined(Q_OS_WIN32)
    /*
     * Another hack to Solve the problem of restoring a minimized frameless window on Windows
     * See http://qt-project.org/forums/viewthread/7081
     */
    QApplication::postEvent(this, new QEvent(QEvent::UpdateRequest), Qt::LowEventPriority);
#endif
    QWidget::showEvent(event);

}

// handle osx's applicationShouldHandleReopen
// QTBUG-10899 OS X: Add support for ApplicationState capability
void MainWindow::checkShowWindow()
{
#if defined(Q_OS_MAC) && (QT_VERSION >= QT_VERSION_CHECK(5, 2, 0))
    if (qApp->applicationState() & Qt::ApplicationActive) {
        if (qApp->activeModalWidget() || qApp->activePopupWidget() || qApp->activeWindow())
            return;
        showWindow();
    }
#endif
}

void MainWindow::createActions()
{
    refresh_qss_action_ = new QAction(QIcon(":/images/toolbar/refresh-gray.png"), tr("Refresh"), this);
    connect(refresh_qss_action_, SIGNAL(triggered()), this, SLOT(refreshQss()));
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_F5) {
        refreshQss();
        return;
    }

    // if (event->key() == Qt::Key_F6) {
    //     showTestDialog(this);
    //     return;
    // }

    QMainWindow::keyPressEvent(event);
}

void MainWindow::showWindow()
{
    showNormal();
    show();
    raise();
    activateWindow();
    // a hack with UIElement application
#ifdef Q_OS_MAC
    utils::mac::orderFrontRegardless(seafApplet->mainWindow()->winId());
#endif
}

void MainWindow::refreshQss()
{
    seafApplet->refreshQss();
}

void MainWindow::writeSettings()
{
    QSettings settings;

    settings.beginGroup("MainWindow");
    settings.setValue("size", size());
    settings.setValue("pos", pos());
    settings.endGroup();
}

QPoint MainWindow::getDefaultPosition()
{
    const QRect screen = QApplication::desktop()->screenGeometry();
    const QPoint top_right = screen.topRight();

    int top_margin = rect().width() + qMin(150, (int)(0.1 * screen.width()));
    int right_margin = qMin(150, (int)(0.1 * screen.width()));
    QPoint default_pos(top_right.x() -top_margin, top_right.y() + right_margin);

    return default_pos;
}

void MainWindow::readSettings()
{
    QPoint pos;
    QSize size;
    QSettings settings;
    settings.beginGroup("MainWindow");

    static bool first_show = true;

    if (first_show && seafApplet->configurator()->firstUse()) {
        pos = getDefaultPosition();
    } else {
        pos = settings.value("pos", getDefaultPosition()).toPoint();
        size = settings.value("size", QSize()).toSize();

        // we don't want to be out of screen
        // at least 1/10 size
        if (isOutsideScreens(QRect(pos, (size.isValid() ? size : QSize(300, 600))/10)))
            pos = getDefaultPosition();
    }

    first_show = false;

    move(pos);
    resize(size);
}
