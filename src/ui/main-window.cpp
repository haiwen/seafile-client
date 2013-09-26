#include <QtGui>
#include <QApplication>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QCoreApplication>

#include "QtAwesome.h"
#include "cloud-view.h"
#include "seafile-applet.h"
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
    setWindowTitle("Seafile");

    cloud_view_ = new CloudView;

    // main_widget_ = new QTabWidget(this);
    // main_widget_->insertTab(INDEX_CLOUD_VIEW,
    //                         cloud_view_,
    //                         awesome->icon(icon_cloud),
    //                         tr("Cloud"));

    setCentralWidget(cloud_view_);

    createActions();
    // createToolBar();
    createMenus();

    centerInScreen();
    refreshQss();
}

void MainWindow::centerInScreen()
{
    // TODO: center the window at startup
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    event->ignore();
    this->hide();
}

void MainWindow::createActions()
{
    about_action_ = new QAction(tr("&About"), this);
    about_action_->setStatusTip(tr("Show the application's About box"));
    connect(about_action_, SIGNAL(triggered()), this, SLOT(about()));

    refresh_qss_action_ = new QAction(QIcon(":/images/refresh.svg"), tr("Refresh"), this);
    connect(refresh_qss_action_, SIGNAL(triggered()), this, SLOT(refreshQss()));
}

void MainWindow::createToolBar()
{
    // tool_bar_ = addToolBar(tr("&main"));
    // tool_bar_->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    // tool_bar_->setFloatable(false);
    // tool_bar_->setMovable(false);
    // tool_bar_->setContextMenuPolicy(Qt::PreventContextMenu);

    // tool_bar_->addAction(cloud_view_->getAccountWidgetAction());
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
}

void MainWindow::createMenus()
{
    help_menu_ = menuBar()->addMenu(tr("&Help"));
    help_menu_->addAction(about_action_);
}

void MainWindow::about()
{
    QMessageBox::about(this, tr("About Seafile"),
                       tr("<h2>Seafile Client 2.0 beta</h2>"
                          "<p>Copyright &copy; 2013 Seafile Ltd."));
}

void MainWindow::refreshQss()
{
    QFile qss(":/qt.css");

    if (!qss.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }
    QTextStream input(&qss);
    QString style = input.readAll();
    qApp->setStyleSheet(style);
}

