#include <cstdio>
#include <QtGlobal>

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
#include <QtWidgets>
#else
#include <QtGui>
#endif
#include <QIcon>
#include <QStackedWidget>
#include <QModelIndex>
#include <QLabel>

#include "seafile-applet.h"
#include "account-mgr.h"
#include "events-list-view.h"
#include "loading-view.h"
#include "events-service.h"
#include "avatar-service.h"
#include "api/api-error.h"

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
    INDEX_EVENTS_VIEW,
};


}


ActivitiesTab::ActivitiesTab(QWidget *parent)
    : TabView(parent)
{
    createEventsView();
    createLoadingView();
    createLoadingFailedView();

    mStack->insertWidget(INDEX_LOADING_VIEW, loading_view_);
    mStack->insertWidget(INDEX_LOADING_FAILED_VIEW, loading_failed_view_);
    mStack->insertWidget(INDEX_EVENTS_VIEW, events_container_view_);

    connect(EventsService::instance(), SIGNAL(refreshSuccess(const std::vector<SeafEvent>&, bool, bool)),
            this, SLOT(refreshEvents(const std::vector<SeafEvent>&, bool, bool)));
    connect(EventsService::instance(), SIGNAL(refreshFailed(const ApiError&)),
            this, SLOT(refreshFailed(const ApiError&)));

    connect(AvatarService::instance(), SIGNAL(avatarUpdated(const QString&, const QImage&)),
            events_list_model_, SLOT(onAvatarUpdated(const QString&, const QImage&)));

    refresh();
}

void ActivitiesTab::loadMoreEvents()
{
    EventsService::instance()->loadMore();
    load_more_btn_->setVisible(false);
    events_loading_view_->setVisible(true);
}

void ActivitiesTab::refreshEvents(const std::vector<SeafEvent>& events,
                                  bool is_loading_more,
                                  bool has_more)
{
    emit activitiesSupported();
    mStack->setCurrentIndex(INDEX_EVENTS_VIEW);

    // XXX: "load more events" for now
    // events_loading_view_->setVisible(false);
    // load_more_btn_->setVisible(has_more);

    const QModelIndex first = events_list_model_->updateEvents(events, is_loading_more);
    if (first.isValid()) {
        events_list_view_->scrollTo(first);
    }
}

void ActivitiesTab::refresh()
{
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

    load_more_btn_ = new QToolButton;
    load_more_btn_->setText(tr("More"));
    load_more_btn_->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Maximum);
    connect(load_more_btn_, SIGNAL(clicked()),
            this, SLOT(loadMoreEvents()));
    load_more_btn_->setVisible(false);
    layout->addWidget(load_more_btn_);

    events_loading_view_ = new LoadingView;
    events_loading_view_->setVisible(false);
    layout->addWidget(events_loading_view_);
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
        text = tr("File Activities are only supported in Seafile Server Professional Edition.");
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
    EventsService::instance()->start();
}

void ActivitiesTab::stopRefresh()
{
    EventsService::instance()->stop();
}
