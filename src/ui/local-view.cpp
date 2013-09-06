#include <vector>
#include <QtGui>
#include <QTimer>

#include "seafile-applet.h"
#include "rpc/rpc-client.h"
#include "rpc/local-repo.h"
#include "local-repos-list-view.h"
#include "local-repos-list-model.h"
#include "local-view.h"

namespace {

const int kRefreshReposInterval = 2000;

}

LocalView::LocalView(QWidget *parent)
    : QWidget(parent),
      in_refresh_(false)
{
    repos_list_ = new LocalReposListView;
    repos_model_ = new LocalReposListModel;
    repos_list_->setModel(repos_model_);

    QStackedLayout *layout = new QStackedLayout;
    layout->addWidget(repos_list_);

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

    std::vector<LocalRepo> repos;
    if (seafApplet->rpcClient()->listLocalRepos(&repos) < 0) {
        // Error
    }

    qDebug("list local repos success\n");

    repos_model_->setRepos(repos);
}
