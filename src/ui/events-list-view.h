#ifndef SEAFILE_CLIENT_UI_EVENTS_LIST_VIEW_H
#define SEAFILE_CLIENT_UI_EVENTS_LIST_VIEW_H

#include <vector>
#include <QListView>
#include <QStandardItem>
#include <QStyledItemDelegate>
#include <QModelIndex>

#include "api/event.h"

class QImage;
class QEvent;

class SeafEvent;

enum {
    EVENT_ITEM_TYPE = QStandardItem::UserType
};

class EventItem : public QStandardItem {
public:
    EventItem(const SeafEvent& event);

    virtual int type() const { return EVENT_ITEM_TYPE; }

    const SeafEvent& event() const { return event_; }

private:

    SeafEvent event_;
};

class EventItemDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit EventItemDelegate(QObject *parent=0);

    void paint(QPainter *painter,
               const QStyleOptionViewItem& option,
               const QModelIndex& index) const;

    QSize sizeHint(const QStyleOptionViewItem& option,
                   const QModelIndex& index) const;

private:
    void paintItem(QPainter *painter,
                   const QStyleOptionViewItem& opt,
                   const EventItem *item) const;

    QSize sizeHintForItem(const QStyleOptionViewItem &option,
                          const EventItem *item) const;

    EventItem* getItem(const QModelIndex &index) const;
};

class EventsListModel : public QStandardItemModel {
    Q_OBJECT
public:
    EventsListModel(QObject *parent=0);

    const QModelIndex updateEvents(const std::vector<SeafEvent>& events, bool is_loading_more);

public slots:
    void onAvatarUpdated(const QString& email, const QImage& img);
};

class EventsListView : public QListView {
    Q_OBJECT
public:
    EventsListView(QWidget *parent=0);

    void updateEvents(const std::vector<SeafEvent>& events, bool is_loading_more);

    bool viewportEvent(QEvent *event);
                                                                                 
private slots:
    void onItemDoubleClicked(const QModelIndex& index);

private:
    Q_DISABLE_COPY(EventsListView)

    EventItem* getItem(const QModelIndex &index) const;
};


#endif // SEAFILE_CLIENT_UI_EVENTS_LIST_VIEW_H
