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
    void hide();

    void readSettings();
    void writeSettings();

protected:
    void createActions();
    bool event(QEvent *event);

private slots:
    void refreshQss();
    void closeEvent(QCloseEvent *event);
    void showEvent(QShowEvent *event);

private:
    QAction *refresh_qss_action_;

    // ToolBar
    QToolBar *tool_bar_;

    QTabWidget *main_widget_;

    CloudView *cloud_view_;
};

#endif  // SEAFILE_MAINWINDOW_H
