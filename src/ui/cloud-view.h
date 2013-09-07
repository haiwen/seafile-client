#ifndef SEAFILE_CLIENT_CLOUD_VIEW_H
#define SEAFILE_CLIENT_CLOUD_VIEW_H

#include <QWidget>
#include "account.h"

class QPoint;
class QMenu;
class QTimer;
class QShowEvent;
class QHideEvent;
class QWidgetAction;
class QToolButton;
class QTreeView;

class ListReposRequest;
class ServerRepo;
class RepoTreeModel;

class CloudView : public QWidget
{
    Q_OBJECT
public:
    CloudView(QWidget *parent=0);
    QWidgetAction *getAccountWidgetAction() { return account_widget_action_; }

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

private:
    Q_DISABLE_COPY(CloudView)

    void prepareAccountButtonMenu();
    void createLoadingView();
    QAction *makeAccountAction(const Account& account);
    void showLoadingView();
    void showRepos();
    bool hasAccount();

    QTreeView *repos_tree_;
    RepoTreeModel *repos_model_;

    QTimer *refresh_timer_;
    ListReposRequest *list_repo_req_;
    bool in_refresh_;

    // FolderDropArea *drop_area_;
    Account current_account_;

    // Account operations
    QAction *add_account_action_;
    QAction *delete_account_action_;
    QAction *switch_account_action_;
    QMenu *account_menu_;
    QWidgetAction *account_widget_action_;
    QToolButton *account_tool_button_;

    QWidget *loading_view_;
};


#endif  // SEAFILE_CLIENT_CLOUD_VIEW_H
