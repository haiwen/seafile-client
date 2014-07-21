#include <QtGlobal>

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
#include <QtWidgets>
#else
#include <QtGui>
#endif
#include <QTimer>
#include <QStackedWidget>

#include "seafile-applet.h"
#include "account-mgr.h"
#include "repo-service.h"
#include "repo-tree-view.h"
#include "repo-tree-model.h"
#include "repo-item-delegate.h"
#include "api/requests.h"
#include "api/server-repo.h"
#include "rpc/local-repo.h"
#include "utils/widget-utils.h"

#include "repos-tab.h"

namespace {

const char *kLoadingFaieldLabelName = "loadingFailedText";

enum {
    INDEX_LOADING_VIEW = 0,
    INDEX_LOADING_FAILED_VIEW,
    INDEX_REPOS_VIEW
};

} // namespace

ReposTab::ReposTab(QWidget *parent)
    : TabView(parent)
{
    createRepoTree();
    createLoadingView();
    createLoadingFailedView();

    mStack->insertWidget(INDEX_LOADING_VIEW, loading_view_);
    mStack->insertWidget(INDEX_LOADING_FAILED_VIEW, loading_failed_view_);
    mStack->insertWidget(INDEX_REPOS_VIEW, repos_tree_);

    RepoService *svc = RepoService::instance();

    connect(svc, SIGNAL(refreshSuccess(const std::vector<ServerRepo>&)),
            this, SLOT(refreshRepos(const std::vector<ServerRepo>&)));
    connect(svc, SIGNAL(refreshFailed(const ApiError&)),
            this, SLOT(refreshReposFailed(const ApiError&)));

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
    loading_view_ = ::newLoadingView();
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

void ReposTab::refreshRepos(const std::vector<ServerRepo>& repos)
{
    repos_model_->setRepos(repos);

    mStack->setCurrentIndex(INDEX_REPOS_VIEW);
}

void ReposTab::refreshReposFailed(const ApiError& error)
{
    qDebug("failed to refresh repos");

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

void ReposTab::refresh()
{
    showLoadingView();
    RepoService::instance()->refresh(true);
}
