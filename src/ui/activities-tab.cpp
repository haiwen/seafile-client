#include <cstdio>
#include <QtGlobal>

#include <QtWidgets>
#include <QIcon>
#include <QStackedWidget>
#include <QModelIndex>
#include <QLabel>

#include "seafile-applet.h"
#include "account-mgr.h"
#include "events-list-view.h"
#include "loading-view.h"
#include "logout-view.h"
#include "events-service.h"
#include "avatar-service.h"
#include "api/api-error.h"
#include "utils/utils.h"

#include "activities-tab.h"

namespace {

//const int kRefreshInterval = 1000 * 60 * 5; // 5 min
const char *kLoadingFailedLabelName = "loadingFailedText";
//const char *kEmptyViewLabelName = "emptyText";
//const char *kAuthHeader = "Authorization";
//const char *kActivitiesUrl = "/api2/html/events/";

enum {
    INDEX_LOADING_VIEW = 0,
    INDEX_LOADING_FAILED_VIEW,
    INDEX_LOGOUT_VIEW,
    INDEX_EVENTS_VIEW,
};


}


ActivitiesTab::ActivitiesTab(QWidget *parent)
    : TabView(parent)
{
    createEventsView();
    createLoadingView();
    createLoadingFailedView();

    //createLogoutView
    logout_view_ = new LogoutView;
    static_cast<LogoutView*>(logout_view_)->setQssStyleForTab();

    mStack->insertWidget(INDEX_LOADING_VIEW, loading_view_);
    mStack->insertWidget(INDEX_LOADING_FAILED_VIEW, loading_failed_view_);
    mStack->insertWidget(INDEX_LOGOUT_VIEW, logout_view_);
    mStack->insertWidget(INDEX_EVENTS_VIEW, events_container_view_);

    connect(EventsService::instance(), SIGNAL(refreshSuccess(const std::vector<SeafEvent>&, bool, bool)),
            this, SLOT(refreshEvents(const std::vector<SeafEvent>&, bool, bool)));
    connect(EventsService::instance(), SIGNAL(refreshFailed(const ApiError&)),
            this, SLOT(refreshFailed(const ApiError&)));

    connect(AvatarService::instance(), SIGNAL(avatarUpdated(const QString&, const QImage&)),
            events_list_model_, SLOT(onAvatarUpdated(const QString&, const QImage&)));

    refresh();
}

void ActivitiesTab::showEvent(QShowEvent *event)
{
    TabView::showEvent(event);
    if (mStack->currentIndex() == INDEX_EVENTS_VIEW) {
        events_list_view_->update();
    }
}

void ActivitiesTab::loadMoreEvents()
{
    EventsService::instance()->loadMore();
}

void ActivitiesTab::refreshEvents(const std::vector<SeafEvent>& events,
                                  bool is_loading_more,
                                  bool has_more)
{
    events_list_model_->removeRow(
        events_list_model_->loadMoreIndex().row());

    mStack->setCurrentIndex(INDEX_EVENTS_VIEW);

    // XXX: "load more events" for now
    const QModelIndex first =
        events_list_model_->updateEvents(events, is_loading_more, has_more);
    if (first.isValid()) {
        events_list_view_->scrollTo(first);
    }

    if (has_more) {
        load_more_btn_ = new LoadMoreButton;
        connect(load_more_btn_, SIGNAL(clicked()),
                this, SLOT(loadMoreEvents()));
        events_list_view_->setIndexWidget(
            events_list_model_->loadMoreIndex(), load_more_btn_);
    }
}

void ActivitiesTab::refresh()
{
    if (!seafApplet->accountManager()->hasAccount() ||
        !seafApplet->accountManager()->accounts().front().isValid()) {
        mStack->setCurrentIndex(INDEX_LOGOUT_VIEW);
        return;
    }
    showLoadingView();

    EventsService::instance()->refresh(true);
}

void ActivitiesTab::createEventsView()
{
    events_container_view_ = new QWidget;
    events_container_view_->setObjectName("EventsContainerView");
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    events_container_view_->setLayout(layout);

    events_list_view_ = new EventsListView;
    layout->addWidget(events_list_view_);

    events_list_model_ = new EventsListModel;
    events_list_view_->setModel(events_list_model_);
}

void ActivitiesTab::createLoadingView()
{
    loading_view_ = new LoadingView;
    static_cast<LoadingView*>(loading_view_)->setQssStyleForTab();
}

void ActivitiesTab::createLoadingFailedView()
{
    loading_failed_view_ = new QWidget(this);

    QVBoxLayout *layout = new QVBoxLayout;
    loading_failed_view_->setLayout(layout);

    loading_failed_text_ = new QLabel;
    loading_failed_text_->setObjectName(kLoadingFailedLabelName);
    loading_failed_text_->setAlignment(Qt::AlignCenter);

    connect(loading_failed_text_, SIGNAL(linkActivated(const QString&)),
            this, SLOT(refresh()));

    layout->addWidget(loading_failed_text_);
}

void ActivitiesTab::showLoadingView()
{
    mStack->setCurrentIndex(INDEX_LOADING_VIEW);
}

void ActivitiesTab::refreshFailed(const ApiError& error)
{
    QString text;
    if (error.type() == ApiError::HTTP_ERROR
        && error.httpErrorCode() == 404) {
        text = tr("File Activities are only supported in %1 Server Professional Edition.").arg(getBrand());
    } else {
        QString link = QString("<a style=\"color:#777\" href=\"#\">%1</a>").arg(tr("retry"));
        text = tr("Failed to get actvities information. "
                  "Please %1").arg(link);
    }

    loading_failed_text_->setText(text);

    mStack->setCurrentIndex(INDEX_LOADING_FAILED_VIEW);
}

void ActivitiesTab::startRefresh()
{
    AccountManager *mgr = seafApplet->accountManager();
    bool has_pro_account = mgr->hasAccount() && mgr->accounts().front().isPro();
    if (has_pro_account)
        EventsService::instance()->start();
}

void ActivitiesTab::stopRefresh()
{
    EventsService::instance()->stop();
}
