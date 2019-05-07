#ifndef SEAFILE_CLIENT_EVENTS_SERVICE_H
#define SEAFILE_CLIENT_EVENTS_SERVICE_H

#include <vector>
#include <QObject>

#include "api/event.h"

class ApiError;
class GetEventsRequest;
class GetFileActivitiesRequest;

class EventsService : public QObject
{
    Q_OBJECT
public:
    static EventsService* instance();

    void start();
    void stop();

    void refresh(bool force);

    void loadMore();

    // accessors
    const std::vector<SeafEvent>& events() const { return events_; }

    bool hasMore() const { return more_offset_ > 0; }

public slots:
    void refresh();

private slots:
    void onRefreshSuccess(const std::vector<SeafEvent>& events, int more_offset);
    void onNewFileActivitiesRefreshSuccess(const std::vector<SeafEvent>& events);
    void onRefreshFailed(const ApiError& error);

signals:
    void refreshSuccess(const std::vector<SeafEvent>& events, bool is_loading_more, bool has_more);
    void refreshFailed(const ApiError& error);

private:
    Q_DISABLE_COPY(EventsService)

    EventsService(QObject *parent=0);
    void sendRequest(bool is_loading_more);

    static EventsService *singleton_;

    const std::vector<SeafEvent> handleEventsOffset(const std::vector<SeafEvent>& new_events);

    GetEventsRequest *get_events_req_;

    GetFileActivitiesRequest *get_file_activities_req_;

    std::vector<SeafEvent> events_;

    bool in_refresh_;

    int more_offset_;
    int next_;
};


#endif // SEAFILE_CLIENT_EVENTS_SERVICE_H
