#ifndef SEAFILE_CLIENT_UI_REPOS_TAB_H
#define SEAFILE_CLIENT_UI_REPOS_TAB_H

#include "tab-view.h"

class QTimer;
class QLineEdit;

class RepoTreeModel;
class RepoFilterProxyModel;
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

protected:
    void startRefresh();
    void stopRefresh();

private slots:
    void refreshRepos(const std::vector<ServerRepo>& repos);
    void refreshReposFailed(const ApiError& error);
    void onFilterTextChanged(const QString& text);

private:
    void createRepoTree();
    void createLoadingView();
    void createLoadingFailedView();
    void showLoadingView();

    RepoTreeModel *repos_model_;
    RepoFilterProxyModel *filter_model_;

    RepoTreeView *repos_tree_;
    QWidget *loading_view_;
    QWidget *loading_failed_view_;

    QLineEdit *filter_text_;

    ListReposRequest *list_repo_req_;
};

#endif // SEAFILE_CLIENT_UI_REPOS_TAB_H
