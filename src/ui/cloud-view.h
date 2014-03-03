#ifndef SEAFILE_CLIENT_CLOUD_VIEW_H
#define SEAFILE_CLIENT_CLOUD_VIEW_H

#include <QWidget>
#include "account.h"
#include "ui_cloud-view.h"
class QPoint;
class QMenu;
class QTimer;
class QShowEvent;
class QHideEvent;
class QToolButton;
class QToolBar;
class QSizeGrip;

class ListReposRequest;
class ServerRepo;
class RepoTreeView;
class RepoTreeModel;
class CloneTasksDialog;
class SeahubMessagesMonitor;
class ApiError;

class CloudView : public QWidget,
                  public Ui::CloudView
{
    Q_OBJECT
public:
    CloudView(QWidget *parent=0);
    const Account& currentAccount() { return current_account_; }

    CloneTasksDialog* cloneTasksDialog();

    QToolButton *seahubMessagesBtn() const { return mSeahubMessagesBtn; }

protected:
    void showEvent(QShowEvent *event);
    void hideEvent(QHideEvent *event);
    void resizeEvent(QResizeEvent *event);
    bool eventFilter(QObject *obj, QEvent *ev);

public slots:
    void showAddAccountDialog();
    void deleteAccount();
    void showCloneTasksDialog();

private slots:
    void refreshRepos();
    void refreshRepos(const std::vector<ServerRepo>& repos);
    void refreshReposFailed(const ApiError& error);
    void setCurrentAccount(const Account&account);
    void updateAccountMenu();
    void onAccountItemClicked();
    void refreshStatusBar();
    void showServerStatusDialog();
    void onRefreshClicked();
    void onMinimizeBtnClicked();
    void onCloseBtnClicked();
    void chooseFolderToSync();

private:
    Q_DISABLE_COPY(CloudView)

    void createLoadingView();
    void createLoadingFailedView();
    void createRepoModelView();
    void prepareAccountButtonMenu();
    void setupHeader();
    void setupDropArea();
    void setupFooter();
    void createToolBar();
    void updateAccountInfoDisplay();
    QAction *makeAccountAction(const Account& account);
    void showLoadingView();
    void showRepos();
    bool hasAccount();
    void refreshServerStatus();
    void refreshTasksInfo();
    void refreshTransferRate();
    void showCreateRepoDialog(const QString& path);

    bool in_refresh_;
    QTimer *refresh_timer_;

    QTimer *refresh_status_bar_timer_;

    RepoTreeModel *repos_model_;

    RepoTreeView *repos_tree_;
    QWidget *loading_view_;
    QWidget *loading_failed_view_;

    QSizeGrip *resizer_;

    ListReposRequest *list_repo_req_;

    // Toolbar and actions
    QToolBar *tool_bar_;
    QAction *refresh_action_;

    // FolderDropArea *drop_area_;
    Account current_account_;

    // Account operations
    QAction *add_account_action_;
    QAction *delete_account_action_;
    QAction *switch_account_action_;
    QMenu *account_menu_;

    CloneTasksDialog* clone_task_dialog_;

    SeahubMessagesMonitor *seahub_messages_monitor_;
};


#endif  // SEAFILE_CLIENT_CLOUD_VIEW_H
