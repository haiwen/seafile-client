#include <QLayout>

#include "seafile-applet.h"
#include "main-window.h"
#include "events-service.h"
#include "event-details-dialog.h"
#include "api/event.h"

#include "events-list-view.h"

EventsListView::EventsListView(QWidget *parent)
    : QListWidget(parent)
{
    connect(this, SIGNAL(itemDoubleClicked(QListWidgetItem *)),
            this, SLOT(onItemDoubleClicked(QListWidgetItem *)));
}

void EventsListView::updateEvents(const std::vector<SeafEvent>& events, bool is_loading_more)
{
    if (!is_loading_more) {
        clear();
    }
    int i = 0, n = events.size();
    for (i = 0; i < n; i++) {
        SeafEvent event = events[i];
        QListWidgetItem *item = new QListWidgetItem(this);
        item->setData(Qt::DisplayRole, event.desc);
        item->setData(Qt::DecorationRole, QIcon(":/images/account.png"));
        item->setData(Qt::UserRole, QVariant::fromValue(event));

        addItem(item);
    }
}

void EventsListView::onItemDoubleClicked(QListWidgetItem* item)
{
    const SeafEvent event = qvariant_cast<SeafEvent>(item->data(Qt::UserRole));

    printf ("event item clicked: %s\n", event.toString().toUtf8().data());

    if (!event.isDetailsDisplayable()) {
        return;
    }

    EventDetailsDialog dialog(event, seafApplet->mainWindow());

    dialog.exec();
}
