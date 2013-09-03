#include <QtGui>
#include <QApplication>
#include <QFile>
#include <QTextStream>

#include "accounts-view.h"
#include "repos-view.h"
#include "seafile-applet.h"
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

    main_widget_ = new QTabWidget(this);
    main_widget_->insertTab(INDEX_ACCOUNTS_VIEW,
                            accounts_view_,
                            QIcon(":/images/account.svg"),
                            tr("Accounts"));

    main_widget_->insertTab(INDEX_REPOS_VIEW,
                            repos_view_,
                            QIcon(":/images/repo.svg"),
                            tr("Repos"));

    connect(main_widget_, SIGNAL(currentChanged(int)), this, SLOT(onViewChanged(int)));

    setCentralWidget(main_widget_);

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

void MainWindow::onViewChanged(int index)
{
    qDebug("index=%d\n", index);
    switch(index) {
    case INDEX_REPOS_VIEW:
        repos_view_->updateRepos();
        break;
    case INDEX_ACCOUNTS_VIEW:
        accounts_view_->refreshAccounts();
        break;
    }
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

    tool_bar_->addAction(refresh_qss_action_);
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
    onViewChanged (main_widget_->currentIndex());
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
