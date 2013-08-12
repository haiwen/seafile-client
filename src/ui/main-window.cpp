#include <QtGui>
#include <QApplication>
#include <QFile>
#include <QTextStream>

#include "accounts-view.h"
#include "repos-view.h"
#include "main-window.h"

namespace {

enum WIDGET_INDEX {
    INDEX_ACCOUNTS_VIEW = 0,
    INDEX_REPOS_VIEW
};

} // namespace

MainWindow::MainWindow()
{
    setWindowIcon(QIcon(":/images/seafile.png"));
    setWindowTitle("Seafile");

    accounts_view_ = new AccountsView;
    repos_view_ = new ReposView;

    main_widget_ = new QStackedWidget(this);
    main_widget_->insertWidget(INDEX_ACCOUNTS_VIEW, accounts_view_);
    main_widget_->insertWidget(INDEX_REPOS_VIEW, repos_view_);

    setCentralWidget(main_widget_);

    createActions();
    createToolBar();
    createMenus();

    centerInScreen();
    refreshQss();
}


void MainWindow::showAccountsView()
{
    main_widget_->setCurrentIndex(INDEX_ACCOUNTS_VIEW);
    show_accounts_btn_->setChecked(true);
    show_repos_btn_->setChecked(false);
}

void MainWindow::showReposView()
{
    main_widget_->setCurrentIndex(INDEX_REPOS_VIEW);
    show_accounts_btn_->setChecked(false);
    show_repos_btn_->setChecked(true);
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

    show_accounts_action_ = new QAction(QIcon(":/images/account.svg"),
                                      tr("Accounts"), this);
    show_accounts_action_->setStatusTip(tr("Show accounts"));

    show_repos_action_ = new QAction(QIcon(":/images/repo.svg"),
                                      tr("Libraries"), this);
    show_repos_action_->setStatusTip(tr("Show libraries"));

    connect(show_accounts_action_, SIGNAL(triggered()), this, SLOT(showAccountsView()));
    connect(show_repos_action_, SIGNAL(triggered()), this, SLOT(showReposView()));

    refresh_qss_action_ = new QAction(QIcon(":/images/refresh.svg"), tr("Refresh"), this);
    connect(refresh_qss_action_, SIGNAL(triggered()), this, SLOT(refreshQss()));
}

void MainWindow::createToolBar()
{
    tool_bar_ = addToolBar(tr("&main"));
    tool_bar_->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

    tool_bar_->addAction(show_accounts_action_);
    tool_bar_->addAction(show_repos_action_);
    tool_bar_->addAction(refresh_qss_action_);

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
