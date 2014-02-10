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

class ListReposRequest;
class AccountInfoRequest;
class ServerRepo;
class AccountInfo;
class RepoTreeView;
class RepoTreeModel;
class CloneTasksDialog;

class CloudView : public QWidget,
                  public Ui::CloudView
{
    Q_OBJECT
public:
    CloudView(QWidget *parent=0);
    const Account& currentAccount() { return current_account_; }

    CloneTasksDialog* cloneTasksDialog();

protected:
    void showEvent(QShowEvent *event);
    void hideEvent(QHideEvent *event);

public slots:
    void showAddAccountDialog();
    void deleteAccount();
    void showCloneTasksDialog();

private slots:
    void refreshRepos();
    void refreshRepos(const std::vector<ServerRepo>& repos);
    void refreshReposFailed();
    void refreshAccountInfo();
    void refreshAccountInfo(const AccountInfo& repos);
    void refreshAccountInfoFailed();
    void setCurrentAccount(const Account&account);
    void updateAccountMenu();
    void onAccountItemClicked();
    void refreshStatusBar();
    void showCreateRepoDialog();
    void showServerStatusDialog();
    void onRefreshClicked();

private:
    Q_DISABLE_COPY(CloudView)

    void createLoadingView();
    void createRepoModelView();
    void prepareAccountButtonMenu();
    void createToolBar();
    void updateAccountInfoDisplay();
    QAction *makeAccountAction(const Account& account);
    void showLoadingView();
    void showRepos();
    bool hasAccount();
    void refreshServerStatus();
    void refreshTasksInfo();
    void refreshTransferRate();

    bool in_refresh_;
    bool in_refresh_account_;
    QTimer *refresh_timer_;

    QTimer *refresh_status_bar_timer_;

    RepoTreeModel *repos_model_;

    RepoTreeView *repos_tree_;
    QWidget *loading_view_;

    ListReposRequest *list_repo_req_;
    AccountInfoRequest *account_info_req_;

    // Toolbar and actions
    QToolBar *tool_bar_;
    QAction *refresh_action_;
    QAction *create_repo_action_;

    // FolderDropArea *drop_area_;
    Account current_account_;

    // Account operations
    QAction *add_account_action_;
    QAction *delete_account_action_;
    QAction *switch_account_action_;
    QMenu *account_menu_;

    CloneTasksDialog* clone_task_dialog_;
};


#endif  // SEAFILE_CLIENT_CLOUD_VIEW_H
