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
class QFile;

class CloudView;

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
    void loadQss(const QString& file);
    
    // Actions
    QAction *about_action_;
    QAction *refresh_qss_action_;

    // Menus
    QMenu *help_menu_;

    // ToolBar
    QToolBar *tool_bar_;

    QTabWidget *main_widget_;

    CloudView *cloud_view_;

    QString style_;
};

#endif  // SEAFILE_MAINWINDOW_H
