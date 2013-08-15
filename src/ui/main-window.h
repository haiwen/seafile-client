#ifndef SEAFILE_MAINWINDOW_H
#define SEAFILE_MAINWINDOW_H

#include <QMainWindow>

class QAction;
class QLabel;
class QListWidget;
class QToolBar;
class QToolButton;

class AccountsView;
class ReposView;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();

    void keyPressEvent(QKeyEvent *event);

protected:
    void createActions();
    void createMenus();
    void createToolBar();
    void centerInScreen();

private slots:
    void about();
    void refreshQss();
    void onViewChanged(int);
    void closeEvent(QCloseEvent *event);

private:
    // Actions
    QAction *about_action_;
    QAction *refresh_qss_action_;

    // Menus
    QMenu *help_menu_;

    // ToolBar
    QToolBar *tool_bar_;

    QTabWidget *main_widget_;

    AccountsView *accounts_view_;
    ReposView *repos_view_;
};

#endif  // SEAFILE_MAINWINDOW_H
