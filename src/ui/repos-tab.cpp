#include <QtGui>
#include <QTimer>
#include <QStackedWidget>
#include <QSortFilterProxyModel>
#include <QLineEdit>

#include "seafile-applet.h"
#include "account-mgr.h"
#include "repo-service.h"
#include "repo-tree-view.h"
#include "repo-tree-model.h"
#include "repo-item-delegate.h"
#include "api/requests.h"
#include "api/server-repo.h"
#include "rpc/local-repo.h"
#include "loading-view.h"

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

    filter_text_ = new QLineEdit;
    filter_text_->setPlaceholderText(tr("Search libraries..."));
    filter_text_->setObjectName("repoNameFilter");
#ifdef Q_WS_MAC
    filter_text_->setAttribute(Qt::WA_MacShowFocusRect, 0);
#endif
    connect(filter_text_, SIGNAL(textChanged(const QString&)),
            this, SLOT(onFilterTextChanged(const QString&)));
    QVBoxLayout *vlayout = (QVBoxLayout *)layout();
    vlayout->insertWidget(0, filter_text_);

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
    repos_tree_ = new RepoTreeView(this);
    repos_model_ = new RepoTreeModel(this);
    repos_model_->setTreeView(repos_tree_);

    filter_model_ = new RepoFilterProxyModel(this);
    filter_model_->setSourceModel(repos_model_);
    filter_model_->setDynamicSortFilter(true);
    repos_tree_->setModel(filter_model_);

    // QAbstractItemView::setItemDelegate won't take ownship of delegate,
    // you need to manage its resource yourself
    repos_tree_->setItemDelegate(new RepoItemDelegate(this));
}

void ReposTab::createLoadingView()
{
    loading_view_ = new LoadingView;
    static_cast<LoadingView*>(loading_view_)->setQssStyleForTab();
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
    onFilterTextChanged(filter_text_->text());
    filter_text_->setVisible(true);
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
    filter_text_->setVisible(false);
    mStack->setCurrentIndex(INDEX_LOADING_VIEW);
}

void ReposTab::refresh()
{
    filter_text_->clear();
    showLoadingView();
    RepoService::instance()->refresh(true);
}

void ReposTab::startRefresh()
{
    RepoService::instance()->start();
}

void ReposTab::stopRefresh()
{
    RepoService::instance()->stop();
}

void ReposTab::onFilterTextChanged(const QString& text)
{
    repos_model_->onFilterTextChanged(text);
    filter_model_->setFilterText(text.trimmed());
    filter_model_->sort(0);
    if (text.isEmpty()) {
        repos_tree_->restoreExpandedCategries();
    } else {
        repos_tree_->expandAll();
    }
}
