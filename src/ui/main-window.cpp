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
#include "utils/utils-win.h"
#include "filebrowser/auto-update-mgr.h"

#include "main-window.h"

namespace {

enum WIDGET_INDEX {
    INDEX_CLOUD_VIEW = 0,
    INDEX_LOCAL_VIEW
};

const int kMinimumTopMargin = 20;
const int kPreferredTopMargin = 150;

const int kMinimumRightMargin = 100;
const int kPreferredRightMargin = 150;

QSize getReasonableWindowSize(const QSize &in)
{
    QSize size;
    const QRect screen = QApplication::desktop()->availableGeometry();
    return QSize(qMin(in.width(), screen.width() - kMinimumRightMargin),
                 qMin(in.height(), screen.height() - kMinimumTopMargin));
}

// Detect if the pos is outside the screens.
bool isOutsideScreens(const QRect &rect) {
    QList<QScreen*> screens = QGuiApplication::screens();
    for (int i = 0; i < screens.size(); ++i) {
        if (screens[i]->availableGeometry().contains(rect))
            return false;
    }
    return true;
}

} // namespace

MainWindow::MainWindow()
{
    setWindowIcon(QIcon(":/images/seafile.png"));
    setWindowTitle(getBrand());

    // Qt::Tool hides the taskbar entry on windows
    // setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint);

    Qt::WindowFlags flags = Qt::Window | Qt::WindowSystemMenuHint;
    if (shouldUseFramelessWindow()) {
        flags |= Qt::FramelessWindowHint;
    } else {
        flags |= Qt::CustomizeWindowHint | Qt::WindowMinimizeButtonHint |
            Qt::WindowCloseButtonHint | Qt::WindowMaximizeButtonHint;
    }

    setWindowFlags(flags);

    cloud_view_ = new CloudView;

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(cloud_view_);

    QWidget *wrapper = new QWidget;
    wrapper->setObjectName("mainWrapper");
    wrapper->setLayout(layout);
    if (shouldUseFramelessWindow()) {
        setAttribute(Qt::WA_TranslucentBackground, true);
    } else {
        wrapper->setStyleSheet("QWidget#mainWrapper {border : 0; border-radius: 0px;}");
    }

    setCentralWidget(wrapper);

    createActions();

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
    event->ignore();
    hide();
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
    if (!shouldUseFramelessWindow()) {
        QWidget::changeEvent(event);
    }
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
    if (shouldUseFramelessWindow()) {
        QApplication::postEvent(this, new QEvent(QEvent::UpdateRequest), Qt::LowEventPriority);
    }
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
    } else if (event->key() == Qt::Key_F6) {
        AutoUpdateManager::instance()->dumpCacheStatus();
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

QPoint MainWindow::getDefaultPosition(const QSize& size)
{
    const QRect screen = QApplication::desktop()->availableGeometry();
    const QPoint top_right = screen.topRight();

    int extra_height = qMax(screen.height() - size.height(), kMinimumTopMargin) / 2;
    int right_margin = rect().width() + qMin(kPreferredRightMargin, (int)(0.1 * screen.width()));
    int top_margin = qMin(qMin(extra_height, kPreferredTopMargin), (int)(0.1 * screen.width()));

    return QPoint(top_right.x() - right_margin, top_right.y() + top_margin);
}

void MainWindow::readSettings()
{
    QPoint pos;
    // Default size of the main window from qt.css.
    const QSize default_size(325, 585);
    QSize size;
    QSettings settings;
    settings.beginGroup("MainWindow");

    static bool first_show = true;

    if (first_show && seafApplet->configurator()->firstUse()) {
        size = default_size;
        pos = getDefaultPosition(default_size);
    } else {
        size = settings.value("size").toSize();
        if (!size.isValid()) {
            size = default_size;
        } else {
            size = getReasonableWindowSize(size);
        }

        pos = settings.value("pos", getDefaultPosition(size)).toPoint();

        // we don't want to be out of screen at least 1/10 size
        if (isOutsideScreens(QRect(pos, size / 10))) {
            pos = getDefaultPosition(size);
        }
    }

    first_show = false;

    move(pos);
    resize(size);
}
