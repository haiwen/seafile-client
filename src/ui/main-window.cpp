#include <QtGui>
#include <QApplication>

#include "account-view.h"
#include "main-window.h"

MainWindow::MainWindow()
{
    createActions();
    createToolBar();
    createMenus();
    setWindowIcon(QIcon(":/images/seafile.png"));
    setWindowTitle("Seafile");

    accounts_view_ = new AccountView(this);
    setCentralWidget(accounts_view_);
    centerInScreen();
}

void MainWindow::centerInScreen()
{
    // TODO: center the window at startup
}

void MainWindow::createActions()
{
    about_action_ = new QAction(tr("&About"), this);
    about_action_->setStatusTip(tr("Show the application's About box"));
    connect(about_action_, SIGNAL(triggered()), this, SLOT(about()));

    show_accounts_action_ = new QAction(QIcon(":/images/account.png"),
                                      tr("Accounts"), this);
    show_accounts_action_->setStatusTip(tr("Show accounts"));
    connect(show_accounts_action_, SIGNAL(triggered()), this, SLOT(showAccounts()));

    show_repos_action_ = new QAction(QIcon(":/images/repos.png"),
                                      tr("Libraries"), this);
    show_repos_action_->setStatusTip(tr("Show libraries"));
    connect(show_repos_action_, SIGNAL(triggered()), this, SLOT(showRepos()));
}

void MainWindow::createToolBar()
{
    tool_bar_ = addToolBar(tr("&main"));
    tool_bar_->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

    tool_bar_->addAction(show_accounts_action_);
    tool_bar_->addAction(show_repos_action_);

    show_accounts_btn_ = dynamic_cast<QToolButton *>(tool_bar_->widgetForAction(show_accounts_action_));
    show_accounts_btn_->setCheckable(true);

    show_repos_btn_ = dynamic_cast<QToolButton *>(tool_bar_->widgetForAction(show_repos_action_));
    show_repos_btn_->setCheckable(true);
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

void MainWindow::showAccounts()
{
    show_accounts_btn_->setChecked(true);
    show_repos_btn_->setChecked(false);
}

void MainWindow::showRepos()
{
    show_accounts_btn_->setChecked(false);
    show_repos_btn_->setChecked(true);
}
