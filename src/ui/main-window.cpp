#include <QtGui>
#include <QApplication>
#include <QDesktopServices>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QCoreApplication>

#include "QtAwesome.h"
#include "cloud-view.h"
#include "seafile-applet.h"
#include "tray-icon.h"
#include "login-dialog.h"
#include "main-window.h"
#include "utils/utils.h"

namespace {

enum WIDGET_INDEX {
    INDEX_CLOUD_VIEW = 0,
    INDEX_LOCAL_VIEW
};

} // namespace


MainWindow::MainWindow()
{
    setWindowIcon(QIcon(":/images/seafile.png"));
    setWindowTitle(SEAFILE_CLIENT_BRAND);

    cloud_view_ = new CloudView;

    // main_widget_ = new QTabWidget(this);
    // main_widget_->insertTab(INDEX_CLOUD_VIEW,
    //                         cloud_view_,
    //                         awesome->icon(icon_cloud),
    //                         tr("Cloud"));

    setCentralWidget(cloud_view_);

    createActions();
}

void MainWindow::hide()
{
    writeSettings();
    QMainWindow::hide();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    hide();
    event->ignore();
}

bool MainWindow::event(QEvent *ev)
{
    if (ev->type() == QEvent::WindowStateChange) {
        QWindowStateChangeEvent *wev = (QWindowStateChangeEvent *)ev;
        if (wev->oldState() != Qt::WindowMinimized) {
            writeSettings();
        }
    }

    return QMainWindow::event(ev);
}

void MainWindow::showEvent(QShowEvent *event)
{
    readSettings();
    QMainWindow::showEvent(event);
}

void MainWindow::createActions()
{
    refresh_qss_action_ = new QAction(QIcon(":/images/refresh.png"), tr("Refresh"), this);
    connect(refresh_qss_action_, SIGNAL(triggered()), this, SLOT(refreshQss()));
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_F5) {
        refreshQss();
        return;
    }

    QMainWindow::keyPressEvent(event);
}

void MainWindow::showWindow()
{
    this->show();
    this->raise();
    this->activateWindow();
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

void MainWindow::readSettings()
{
    QSettings settings;

    settings.beginGroup("MainWindow");
    if (settings.contains("size")) {
        resize(settings.value("size", QSize()).toSize());
    }

    if (settings.contains("pos")) {
        move(settings.value("pos", QPoint()).toPoint());
    }
    settings.endGroup();
}
