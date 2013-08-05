#ifndef SEAFILE_MAINWINDOW_H
#define SEAFILE_MAINWINDOW_H

#include <QMainWindow>

class QAction;
class QLabel;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();

protected:
    void createActions();
    void createMenus();
private slots:
    void about();

private:
    QAction *mAboutAction;
    QMenu *mHelpMenu;
};

#endif  // SEAFILE_MAINWINDOW_H
