#include <QtGui>
#include <QTimer>
#include <QStackedWidget>

#include "seafile-applet.h"
#include "account-mgr.h"
#include "repo-tree-view.h"
#include "repo-tree-model.h"
#include "repo-item-delegate.h"
#include "api/requests.h"
#include "api/server-repo.h"
#include "rpc/local-repo.h"

#include "repos-tab.h"

namespace {

const int kRefreshReposInterval = 1000 * 60 * 5; // 5 min
const char *kLoadingFaieldLabelName = "loadingFailedText";

enum {
    INDEX_LOADING_VIEW = 0,
    INDEX_LOADING_FAILED_VIEW,
    INDEX_REPOS_VIEW
};

}

ReposTab::ReposTab(QWidget *parent)
    : TabView(parent),
      in_refresh_(false)
{
    createRepoTree();
    createLoadingView();
    createLoadingFailedView();

    mStack->insertWidget(INDEX_LOADING_VIEW, loading_view_);
    mStack->insertWidget(INDEX_LOADING_FAILED_VIEW, loading_failed_view_);
    mStack->insertWidget(INDEX_REPOS_VIEW, repos_tree_);

    refresh_timer_ = new QTimer(this);
    connect(refresh_timer_, SIGNAL(timeout()), this, SLOT(refresh()));
    refresh_timer_->start(kRefreshReposInterval);

    list_repo_req_ = NULL;

    refresh();
}

void ReposTab::createRepoTree()
{
    repos_tree_ = new RepoTreeView;
    repos_model_ = new RepoTreeModel;
    repos_model_->setTreeView(repos_tree_);

    repos_tree_->setModel(repos_model_);
    repos_tree_->setItemDelegate(new RepoItemDelegate);
}

void ReposTab::createLoadingView()
{
    loading_view_ = new QWidget(this);

    QVBoxLayout *layout = new QVBoxLayout;
    loading_view_->setLayout(layout);

    QMovie *gif = new QMovie(":/images/loading.gif");
    QLabel *label = new QLabel;
    label->setMovie(gif);
    label->setAlignment(Qt::AlignCenter);
    gif->start();

    layout->addWidget(label);
}

void ReposTab::createLoadingFailedView()
{
    loading_failed_view_ = new QWidget(this);

    QVBoxLayout *layout = new QVBoxLayout;
    loading_failed_view_->setLayout(layout);

    QLabel *label = new QLabel;
    label->setObjectName(kLoadingFaieldLabelName);
    QString link = QString("<a style=\"color:#777\" href=\"#\">%1</a>").arg(tr("retry"));
    QString label_text = tr("Failed to get libraries information<br/>"
                            "Please %1").arg(link);
    label->setText(label_text);
    label->setAlignment(Qt::AlignCenter);

    connect(label, SIGNAL(linkActivated(const QString&)),
            this, SLOT(refresh()));

    layout->addWidget(label);
}

void ReposTab::refresh()
{
    if (in_refresh_) {
        return;
    }

    showLoadingView();
    AccountManager *account_mgr = seafApplet->accountManager();

    const std::vector<Account>& accounts = seafApplet->accountManager()->accounts();
    if (accounts.empty()) {
        in_refresh_ = false;
        return;
    }

    in_refresh_ = true;

    if (list_repo_req_) {
        delete list_repo_req_;
    }

    list_repo_req_ = new ListReposRequest(accounts[0]);
    connect(list_repo_req_, SIGNAL(success(const std::vector<ServerRepo>&)),
            this, SLOT(refreshRepos(const std::vector<ServerRepo>&)));
    connect(list_repo_req_, SIGNAL(failed(const ApiError&)),
            this, SLOT(refreshReposFailed(const ApiError&)));
    list_repo_req_->send();
}

void ReposTab::refreshRepos(const std::vector<ServerRepo>& repos)
{
    in_refresh_ = false;
    repos_model_->setRepos(repos);

    list_repo_req_->deleteLater();
    list_repo_req_ = NULL;

    mStack->setCurrentIndex(INDEX_REPOS_VIEW);
}

void ReposTab::refreshReposFailed(const ApiError& error)
{
    qDebug("failed to refresh repos");
    in_refresh_ = false;

    if (mStack->currentIndex() == INDEX_LOADING_VIEW) {
        mStack->setCurrentIndex(INDEX_LOADING_FAILED_VIEW);
    }
}

std::vector<QAction*> ReposTab::getToolBarActions()
{
    return repos_tree_->getToolBarActions();
}

void ReposTab::showLoadingView()
{
    mStack->setCurrentIndex(INDEX_LOADING_VIEW);
}
