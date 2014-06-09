#ifndef SEAFILE_CLIENT_UI_ACTIVITIES_TAB_H
#define SEAFILE_CLIENT_UI_ACTIVITIES_TAB_H

#include <vector>
#include <QList>

#include "tab-view.h"

class QSslError;
class QUrl;
class QNetworkRequest;
class QNetworkReply;
class QToolButton;

class SeafEvent;
class Account;
class ApiError;
class EventsListView;

/**
 * The activities tab
 */
class ActivitiesTab : public TabView {
    Q_OBJECT
public:
    explicit ActivitiesTab(QWidget *parent=0);

public slots:
    void refresh();

private slots:
    void refreshEvents(const std::vector<SeafEvent>& events,
                       bool is_loading_more,
                       bool has_more);
    void loadMoreEvents();

private:
    void createEventsView();
    void createLoadingView();
    void createLoadingFailedView();
    void showLoadingView();
    void loadPage(const Account& account);

    QWidget *loading_view_;
    QWidget *loading_failed_view_;

    QWidget *events_container_view_;
    EventsListView *events_list_view_;
    QWidget *events_loading_view_;
    QToolButton *load_more_btn_;
};

#endif // SEAFILE_CLIENT_UI_ACTIVITIES_TAB_H
