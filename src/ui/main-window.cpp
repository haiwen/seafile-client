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

MainWindow::~MainWindow()
{
    writeSettings();
}

void MainWindow::centerInScreen()
{
    // TODO: center the window at startup
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    writeSettings();
    event->ignore();
    this->hide();
}

void MainWindow::showEvent(QShowEvent *event)
{
    readSettings();
    QMainWindow::showEvent(event);
}

void MainWindow::createActions()
{
    about_action_ = new QAction(tr("&About"), this);
    about_action_->setStatusTip(tr("Show the application's About box"));
    connect(about_action_, SIGNAL(triggered()), this, SLOT(about()));

    open_help_action_ = new QAction(tr("&View online help"), this);
    open_help_action_->setStatusTip(tr("open seafile online help"));
    connect(open_help_action_, SIGNAL(triggered()), this, SLOT(openHelp()));

    settings_action_ = new QAction(tr("&Settings"), this);
    settings_action_->setStatusTip(tr("Edit seafile settings"));
    connect(settings_action_, SIGNAL(triggered()), this, SLOT(showSettingsDialog()));

    refresh_qss_action_ = new QAction(QIcon(":/images/refresh.png"), tr("Refresh"), this);
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
    edit_menu_ = menuBar()->addMenu(tr("&Edit"));
    edit_menu_->addAction(settings_action_);

    help_menu_ = menuBar()->addMenu(tr("&Help"));
    help_menu_->addAction(about_action_);
    help_menu_->addAction(open_help_action_);
}

#define STR(s)     #s
#define STRINGIZE(x) STR(x)

void MainWindow::about()
{
    QMessageBox::about(this, tr("About Seafile"),
#ifdef XCODE_APP
                       tr("<h2>Seafile Client "STRINGIZE(SEAFILE_CLIENT_VERSION)"</h2>"
#else
                       tr("<h2>Seafile Client "SEAFILE_CLIENT_VERSION"</h2>"
#endif
                          "<p>Copyright &copy; 2013 Seafile Ltd."));
}

void MainWindow::showSettingsDialog()
{
    seafApplet->trayIcon()->showSettingsWindow();
}

void MainWindow::openHelp()
{
    QDesktopServices::openUrl(QUrl("http://seafile.com/en/help/install_v2/"));
}

void MainWindow::loadQss(const QString& path)
{
    QFile file(path);
    if (!QFileInfo(file).exists()) {
        return;
    }
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }

    QTextStream input(&file);
    style_ += "\n";
    style_ += input.readAll();
    qApp->setStyleSheet(style_);
}

void MainWindow::refreshQss()
{
    style_.clear();
    loadQss(":/qt.css");
    loadQss("qt.css");

#if defined(Q_WS_WIN)
    loadQss(":/qt-win.css");
    loadQss("qt-win.css");
#elif defined(Q_WS_X11)
    loadQss(":/qt-linux.css");
    loadQss("qt-linux.css");
#else
    loadQss(":/qt-mac.css");
    loadQss("qt-mac.css");
#endif
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
