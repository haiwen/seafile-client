#ifndef SEAFILE_MAINWINDOW_H
#define SEAFILE_MAINWINDOW_H

#include <QMainWindow>

class QAction;
class QToolBar;
class QResizeEvent;
class QSize;

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
    void changeEvent(QEvent *event);

private slots:
    void refreshQss();
    void closeEvent(QCloseEvent *event);
    void showEvent(QShowEvent *event);
    void checkShowWindow(); //dummy slot if qt version is lower than 5.2.0

private:
    Q_DISABLE_COPY(MainWindow)

    QPoint getDefaultPosition(const QSize& size);

    QAction *refresh_qss_action_;

    // ToolBar
    QToolBar *tool_bar_;

    QTabWidget *main_widget_;

    CloudView *cloud_view_;
};

#endif  // SEAFILE_MAINWINDOW_H
