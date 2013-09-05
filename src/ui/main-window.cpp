#include <QtGui>
#include <QApplication>
#include <QFile>
#include <QTextStream>
#include <QToolButton>
#include <QWidgetAction>

#include "QtAwesome.h"
#include "cloud-view.h"
#include "local-view.h"
#include "seafile-applet.h"
#include "login-dialog.h"
#include "main-window.h"

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
    local_view_ = new LocalView;

    main_widget_ = new QTabWidget(this);
    main_widget_->insertTab(INDEX_CLOUD_VIEW,
                            cloud_view_,
                            awesome->icon(icon_cloud),
                            tr("Cloud"));

    main_widget_->insertTab(INDEX_LOCAL_VIEW,
                            local_view_,
                            QIcon(":/images/repo.svg"),
                            tr("Local"));

    setCentralWidget(main_widget_);
    createActions();
    prepareAccountButtonMenu();
    
    createToolBar();
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
    tool_bar_ = addToolBar(tr("&main"));
    tool_bar_->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    tool_bar_->setFloatable(false);
    tool_bar_->setMovable(false);
    tool_bar_->setContextMenuPolicy(Qt::PreventContextMenu);

    tool_bar_->addAction(account_widget_action_);
}

void MainWindow::prepareAccountButtonMenu() {
    accout_op_menu_ = new QMenu;
    switch_account_action_ = new QAction(tr("Switch account"), this);
    add_account_action_ = new QAction(tr("Add an account"), this);
    delete_account_action_ = new QAction(tr("Delete this account"), this);

    connect(switch_account_action_, SIGNAL(triggered()), cloud_view_, SLOT(showSwitchAccountDialog()));
    connect(add_account_action_, SIGNAL(triggered()), cloud_view_, SLOT(showAddAccountDialog()));
    connect(delete_account_action_, SIGNAL(triggered()), cloud_view_, SLOT(deleteAccount()));

    accout_op_menu_->addAction(switch_account_action_);
    accout_op_menu_->addAction(add_account_action_);
    accout_op_menu_->addAction(delete_account_action_);

    account_tool_button_ = new QToolButton(this);
    account_tool_button_->setMenu(accout_op_menu_);
    account_tool_button_->setPopupMode(QToolButton::InstantPopup);
    account_tool_button_->setIcon(QIcon(":/images/account.png"));

    account_widget_action_ = new QWidgetAction(this);
    account_widget_action_->setDefaultWidget(account_tool_button_);
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
                       tr("<h2>Seafile Client 1.8</h2>"
                          "<p>Copyright &copy; 2008 Seafile Inc."));
}

void MainWindow::refreshQss()
{
    QFile qss(QDir::current().filePath("qt.css"));

    if (!qss.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug("failed to open qt.css\n");
        return;
    }
    QTextStream input(&qss);
    QString style = input.readAll();
    qApp->setStyleSheet(style);
}

