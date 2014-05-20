#ifndef SEAFILE_CLIENT_UI_REPOS_TAB_H
#define SEAFILE_CLIENT_UI_REPOS_TAB_H

#include "tab-view.h"

class QTimer;

class RepoTreeModel;
class RepoTreeView;
class ServerRepo;
class ListReposRequest;
class ApiError;

/**
 * The repos list tab
 */
class ReposTab : public TabView {
    Q_OBJECT
public:
    explicit ReposTab(QWidget *parent=0);

    std::vector<QAction*> getToolBarActions();

public slots:
    void refresh();

private slots:
    void refreshRepos(const std::vector<ServerRepo>& repos);
    void refreshReposFailed(const ApiError& error);

private:
    void createRepoTree();
    void createLoadingView();
    void createLoadingFailedView();
    void showLoadingView();
    
    QTimer *refresh_timer_;
    bool in_refresh_;

    RepoTreeModel *repos_model_;

    RepoTreeView *repos_tree_;
    QWidget *loading_view_;
    QWidget *loading_failed_view_;

    ListReposRequest *list_repo_req_;
};

#endif // SEAFILE_CLIENT_UI_REPOS_TAB_H
