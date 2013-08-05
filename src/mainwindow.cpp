#include <QtGui>

#include "mainwindow.h"

MainWindow::MainWindow()
{
    createActions();
    createMenus();
    setWindowIcon(QIcon(":/images/seafile.png"));
}

void MainWindow::createActions()
{
    mAboutAction = new QAction(tr("&About"), this);
    mAboutAction->setStatusTip(tr("Show the application's About box"));
    connect(mAboutAction, SIGNAL(triggered()), this, SLOT(about()));
}

void MainWindow::createMenus()
{
    mHelpMenu = menuBar()->addMenu(tr("&Help"));
    mHelpMenu->addAction(mAboutAction);
}

void MainWindow::about()
{
    QMessageBox::about(this, tr("About Seafile"),
                       tr("<h2>Seafile Client 1.8</h2>"
                          "<p>Copyright &copy; 2008 Seafile Inc."));
}
