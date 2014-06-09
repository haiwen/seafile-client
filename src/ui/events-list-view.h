#ifndef SEAFILE_CLIENT_UI_EVENTS_LIST_VIEW_H
#define SEAFILE_CLIENT_UI_EVENTS_LIST_VIEW_H

#include <vector>
#include <QListWidget>

class SeafEvent;
class QListWidgetItem;

class EventsListView : public QListWidget {
    Q_OBJECT
public:
    EventsListView(QWidget *parent=0);

    void updateEvents(const std::vector<SeafEvent>& events, bool is_loading_more);
private slots:
    void onItemDoubleClicked(QListWidgetItem* item);

private:
    Q_DISABLE_COPY(EventsListView)
    
};

#endif // SEAFILE_CLIENT_UI_EVENTS_LIST_VIEW_H
