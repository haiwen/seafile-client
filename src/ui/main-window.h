#ifndef SEAFILE_MAINWINDOW_H
#define SEAFILE_MAINWINDOW_H

#include <QMainWindow>

class QAction;
class QLabel;
class QListWidget;
class QToolBar;
class QToolButton;
class QStackedLayout;
class AccountView;

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
    void showAccounts();
    void showRepos();

private:
    QAction *about_action_;
    QAction *show_accounts_action_;
    QAction *show_repos_action_;

    QMenu *help_menu_;
    QToolBar *tool_bar_;
    QToolButton *show_accounts_btn_;
    QToolButton *show_repos_btn_;

    QStackedLayout *stacked_layout_;

    AccountView *accounts_view_;
};

#endif  // SEAFILE_MAINWINDOW_H
