#ifndef SEAFILE_MAINWINDOW_H
#define SEAFILE_MAINWINDOW_H

#include <QMainWindow>

class QAction;
class QLabel;
class QListWidget;
class QToolBar;
class QToolButton;
class QWidgetAction;
class QMenu;

class CloudView;
class LocalView;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();

    void keyPressEvent(QKeyEvent *event);
    void showWindow();

protected:
    void createActions();
    void createMenus();
    void createToolBar();
    void centerInScreen();

private slots:
    void about();
    void refreshQss();
    void closeEvent(QCloseEvent *event);

private:
    void prepareAccountButtonMenu();
    
    // Actions
    QAction *about_action_;
    QAction *refresh_qss_action_;

    // Menus
    QMenu *help_menu_;

    // ToolBar
    QToolBar *tool_bar_;

    // Account operations
    QAction *add_account_action_;
    QAction *delete_account_action_;
    QAction *switch_account_action_;
    QMenu *accout_op_menu_;
    QWidgetAction *account_widget_action_;
    QToolButton *account_tool_button_;

    QTabWidget *main_widget_;

    CloudView *cloud_view_;
    LocalView *local_view_;
};

#endif  // SEAFILE_MAINWINDOW_H
