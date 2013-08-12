#ifndef SEAFILE_MAINWINDOW_H
#define SEAFILE_MAINWINDOW_H

#include <QMainWindow>

class QAction;
class QLabel;
class QListWidget;
class QToolBar;
class QToolButton;
class QStackedWidget;

class AccountsView;
class ReposView;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();

protected:
    void createActions();
    void createMenus();
    void createToolBar();
    void centerInScreen();

private slots:
    void about();
    void refreshQss();
    void showAccountsView();
    void showReposView();

private:
    // Actions
    QAction *about_action_;
    QAction *show_accounts_action_;
    QAction *show_repos_action_;
    QAction *refresh_qss_action_;

    // Menus
    QMenu *help_menu_;

    // ToolBar
    QToolBar *tool_bar_;

    QToolButton *show_accounts_btn_;
    QToolButton *show_repos_btn_;

    QStackedWidget *main_widget_;

    AccountsView *accounts_view_;
    ReposView *repos_view_;
};

#endif  // SEAFILE_MAINWINDOW_H
