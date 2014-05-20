#ifndef SEAFILE_CLIENT_CLOUD_VIEW_H
#define SEAFILE_CLIENT_CLOUD_VIEW_H

#include <QWidget>
#include "ui_cloud-view.h"

class QTimer;
class QShowEvent;
class QHideEvent;
class QToolButton;
class QToolBar;
class QSizeGrip;
class QTabWidget;

class SeafileTabWidget;
class ReposTab;
class StarredFilesTab;
class ActivitiesTab;
class CloneTasksDialog;
class SeahubMessagesMonitor;
class AccountView;

class CloudView : public QWidget,
                  public Ui::CloudView
{
    Q_OBJECT
public:
    CloudView(QWidget *parent=0);

    CloneTasksDialog* cloneTasksDialog();

    // QToolButton *seahubMessagesBtn() const { return mSeahubMessagesBtn; }

protected:
    void showEvent(QShowEvent *event);
    void hideEvent(QHideEvent *event);
    void resizeEvent(QResizeEvent *event);
    bool eventFilter(QObject *obj, QEvent *ev);

public slots:
    void showCloneTasksDialog();

private slots:
    void refreshStatusBar();
    void showServerStatusDialog();
    void onRefreshClicked();
    void onMinimizeBtnClicked();
    void onCloseBtnClicked();
    void chooseFolderToSync();
    void onAccountsChanged();

private:
    Q_DISABLE_COPY(CloudView)

    bool hasAccount();

    void setupHeader();
    void createAccountView();
    void createToolBar();
    void createTabs();
    void setupDropArea();
    void setupFooter();

    void refreshServerStatus();
    void refreshTasksInfo();
    void refreshTransferRate();
    void showCreateRepoDialog(const QString& path);

    QTimer *refresh_status_bar_timer_;

    AccountView *account_view_;

    // Toolbar and actions
    QToolBar *tool_bar_;
    QAction *refresh_action_;

    SeafileTabWidget *tabs_;
    ReposTab *repos_tab_;
    StarredFilesTab *starred_files_tab_;
    ActivitiesTab *activities_tab_;

    QSizeGrip *resizer_;

    CloneTasksDialog* clone_task_dialog_;

    // SeahubMessagesMonitor *seahub_messages_monitor_;
};


#endif  // SEAFILE_CLIENT_CLOUD_VIEW_H
