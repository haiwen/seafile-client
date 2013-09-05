#include <vector>
#include <QtGui>
#include <QTimer>

#include "seafile-applet.h"
#include "rpc/rpc-request.h"
#include "rpc/local-repo.h"
#include "local-repos-list-view.h"
#include "local-repos-list-model.h"
#include "local-view.h"

namespace {

const int kRefreshReposInterval = 1000;

}

LocalView::LocalView(QWidget *parent)
    : QWidget(parent),
      in_refresh_(false)
{
    repos_list_ = new LocalReposListView;
    repos_model_ = new LocalReposListModel;
    repos_list_->setModel(repos_model_);

    loading_view_ = new QWidget;

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(repos_list_);
    layout->addWidget(loading_view_);

    setLayout(layout);

    refresh_timer_ = new QTimer(this);
    connect(refresh_timer_, SIGNAL(timeout()), this, SLOT(refreshRepos()));
}

void LocalView::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    refreshRepos();
    refresh_timer_->start(kRefreshReposInterval);
}

void LocalView::hideEvent(QHideEvent *event) {
    QWidget::hideEvent(event);
    refresh_timer_->stop();
}

void LocalView::refreshRepos()
{
    if (in_refresh_) {
        return;
    }

    SeafileRpcRequest *req = new SeafileRpcRequest();

    connect(req, SIGNAL(listLocalReposSignal(const std::vector<LocalRepo>&, bool)),
            this, SLOT(refreshRepos(const std::vector<LocalRepo>&, bool)));

    in_refresh_ = true;

    req->listLocalRepos();
}

void LocalView::refreshRepos(const std::vector<LocalRepo>& repos, bool result)
{
    if (result) {
        repos_model_->setRepos(repos);
    }

    in_refresh_ = false;
}
