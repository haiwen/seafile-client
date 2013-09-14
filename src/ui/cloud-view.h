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

class ListReposRequest;
class ServerRepo;
class RepoTreeView;
class RepoTreeModel;

class CloudView : public QWidget,
                  public Ui::CloudView
{
    Q_OBJECT
public:
    CloudView(QWidget *parent=0);
    const Account& currentAccount() { return current_account_; }

protected:
    void showEvent(QShowEvent *event);
    void hideEvent(QHideEvent *event);

public slots:
    void showAddAccountDialog();
    void deleteAccount();

private slots:
    void refreshRepos();
    void refreshRepos(const std::vector<ServerRepo>& repos);
    void refreshReposFailed();
    void setCurrentAccount(const Account&account);
    void updateAccountMenu();
    void onAccountItemClicked();
    void showCloneTasksDialog();
    void refreshTasksInfo();

private:
    Q_DISABLE_COPY(CloudView)

    void createLoadingView();
    void createRepoModelView();
    void prepareAccountButtonMenu();
    void updateAccountInfoDisplay();
    QAction *makeAccountAction(const Account& account);
    void showLoadingView();
    void showRepos();
    bool hasAccount();

    bool in_refresh_;
    QTimer *refresh_timer_;

    QTimer *refresh_tasks_info_timer_;

    RepoTreeModel *repos_model_;

    RepoTreeView *repos_tree_;
    QWidget *loading_view_;

    ListReposRequest *list_repo_req_;

    // FolderDropArea *drop_area_;
    Account current_account_;

    // Account operations
    QAction *add_account_action_;
    QAction *delete_account_action_;
    QAction *switch_account_action_;
    QMenu *account_menu_;
};


#endif  // SEAFILE_CLIENT_CLOUD_VIEW_H
