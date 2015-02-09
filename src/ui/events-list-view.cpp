#include <QPainter>
#include <QApplication>
#include <QPixmap>
#include <QToolTip>
#include <QLayout>
#include <QDebug>

#include "seafile-applet.h"
#include "main-window.h"
#include "events-service.h"
#include "avatar-service.h"
#include "event-details-dialog.h"
#include "utils/paint-utils.h"
#include "utils/utils.h"
#include "api/event.h"

#include "events-list-view.h"

namespace {

/**
            nick date
   icon
            description
 */

const int kMarginLeft = 5;
//const int kMarginRight = 5;
const int kMarginTop = 5;
const int kMarginBottom = 5;
const int kPadding = 5;

const int kAvatarHeight = 36;
const int kAvatarWidth = 36;
//const int kNickWidth = 210;
const int kNickHeight = 30;

const int kMarginBetweenAvatarAndNick = 10;

const char *kNickColor = "#D8AC8F";
const char *kNickColorHighlighted = "#D8AC8F";
const char *kDescriptionColor = "#3F3F3F";
const char *kDescriptionColorHighlighted = "#544D49";
const int kNickFontSize = 16;
const int kDescriptionFontSize = 13;

const char *kEventItemBackgroundColor = "white";
const char *kEventItemBackgroundColorHighlighted = "#F9E0C7";

const int kTimeWidth = 100;
const int kTimeHeight = 30;
const int kTimeFontSize = 13;

const char *kTimeColor = "#959595";
const char *kTimeColorHighlighted = "#9D9B9A";

const int kMarginBetweenNickAndTime = 10;


const char *kItemBottomBorderColor = "#EEE";

} // namespace



EventItem::EventItem(const SeafEvent& event)
    : event_(event)
{
}

EventItemDelegate::EventItemDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

void EventItemDelegate::paint(QPainter *painter,
                              const QStyleOptionViewItem& option,
                              const QModelIndex& index) const
{
    QBrush backBrush;
    bool selected = false;
    EventItem *item = getItem(index);
    const SeafEvent& event = item->event();
    QString time_text = translateCommitTime(event.timestamp);

    if (option.state & (QStyle::State_HasFocus | QStyle::State_Selected)) {
        backBrush = QColor(kEventItemBackgroundColorHighlighted);
        selected = true;

    } else {
        backBrush = QColor(kEventItemBackgroundColor);
    }

    painter->save();
    painter->fillRect(option.rect, backBrush);
    painter->restore();

    // paint avatar
    QImage avatar;
    if (!event.anonymous) {
        avatar = AvatarService::instance()->getAvatar(event.author);
    }
    if (avatar.size() != QSize(kAvatarWidth, kAvatarHeight)) {
        avatar = devicePixelRatio() > 1 ?
          QImage(":/images/account@2x.png") : QImage(":/images/account.png");
    }

    QRect actualRect(0, 0, kAvatarWidth * devicePixelRatio() , kAvatarHeight * devicePixelRatio());
    avatar.scaled(actualRect.size());
    QImage masked_image(actualRect.size(), QImage::Format_ARGB32_Premultiplied);
    masked_image.fill(Qt::transparent);
    QPainter mask_painter;
    mask_painter.begin(&masked_image);
    mask_painter.setRenderHint(QPainter::Antialiasing);
    mask_painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    mask_painter.setPen(Qt::NoPen);
    mask_painter.setBrush(Qt::white);
    mask_painter.drawEllipse(actualRect);
    mask_painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    mask_painter.drawImage(actualRect, avatar);
    mask_painter.setCompositionMode(QPainter::CompositionMode_DestinationOver);
    mask_painter.fillRect(actualRect, Qt::transparent);
    mask_painter.end();
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    masked_image.setDevicePixelRatio(devicePixelRatio());
#endif // QT5

    QPoint avatar_pos(kMarginLeft + kPadding, kMarginTop + kPadding);
    avatar_pos += option.rect.topLeft();
    painter->save();
    painter->drawImage(avatar_pos, masked_image);
    painter->restore();

    int time_width = qMin(kTimeWidth,
        ::textWidthInFont(time_text,
            changeFontSize(painter->font(), kTimeFontSize)));
    int nick_width = option.rect.width() - kAvatarWidth - kMarginBetweenAvatarAndNick
        - time_width - kMarginBetweenNickAndTime - kPadding * 2;
    nick_width = qMin(nick_width,
                      ::textWidthInFont(event.nick,
                            changeFontSize(painter->font(), kNickFontSize)));

    // Paint nick name
    QPoint nick_pos = avatar_pos + QPoint(kAvatarWidth + kMarginBetweenAvatarAndNick, 0);
    QRect nick_rect(nick_pos, QSize(nick_width, kNickHeight));
    painter->save();
    painter->setPen(QColor(selected ? kNickColorHighlighted : kNickColor));
    painter->setFont(changeFontSize(painter->font(), kNickFontSize));
    painter->drawText(nick_rect,
                      Qt::AlignLeft | Qt::AlignTop,
                      fitTextToWidth(event.nick, option.font, nick_width),
                      &nick_rect);
    painter->restore();

    // Paint event time
    painter->save();
    QPoint time_pos = nick_pos + QPoint(nick_width + kMarginBetweenNickAndTime, 0);
    QRect time_rect(time_pos, QSize(time_width, kTimeHeight));
    painter->setPen(QColor(selected ? kTimeColorHighlighted : kTimeColor));
    painter->setFont(changeFontSize(painter->font(), kTimeFontSize));

    painter->drawText(time_rect,
                      Qt::AlignLeft | Qt::AlignTop,
                      time_text,
                      &time_rect);
    painter->restore();

    // Paint description
    painter->save();
    QPoint event_desc_pos = nick_rect.bottomLeft() + QPoint(0, 5);

    int desc_width = option.rect.width() - kAvatarWidth - kMarginBetweenAvatarAndNick - kPadding * 2;

    QRect event_desc_rect(event_desc_pos, QSize(desc_width, kNickHeight));
    painter->setFont(changeFontSize(painter->font(), kDescriptionFontSize));
    painter->setPen(QColor(selected ? kDescriptionColorHighlighted : kDescriptionColor));

    QString desc = event.desc;
    desc.replace(QChar('\n'), QChar(' '));
    painter->drawText(event_desc_rect,
                      Qt::AlignLeft | Qt::AlignTop,
                      fitTextToWidth(desc, option.font, desc_width),
                      &event_desc_rect);
    painter->restore();

    // Draw the bottom border lines
    painter->save();
    painter->setPen(QPen(QColor(kItemBottomBorderColor), 1, Qt::SolidLine));
    painter->drawLine(option.rect.bottomLeft(), option.rect.bottomRight());
    painter->restore();
}

QSize EventItemDelegate::sizeHint(const QStyleOptionViewItem& option,
                                  const QModelIndex& index) const
{
    int height = kAvatarHeight + kPadding * 2 + kMarginTop + kMarginBottom;
    return QSize(option.rect.width(), height);
}

EventItem*
EventItemDelegate::getItem(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return NULL;
    }
    const EventsListModel *model = (const EventsListModel*)index.model();
    QStandardItem *qitem = model->itemFromIndex(index);
    if (qitem->type() == EVENT_ITEM_TYPE) {
        return (EventItem *)qitem;
    }
    return NULL;
}

EventsListView::EventsListView(QWidget *parent)
    : QListView(parent)
{
    setItemDelegate(new EventItemDelegate);
    connect(this, SIGNAL(doubleClicked(const QModelIndex&)),
            this, SLOT(onItemDoubleClicked(const QModelIndex&)));

    setEditTriggers(QAbstractItemView::NoEditTriggers);
}

EventItem*
EventsListView::getItem(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return NULL;
    }
    const EventsListModel *model = (const EventsListModel*)index.model();
    QStandardItem *qitem = model->itemFromIndex(index);
    if (qitem->type() == EVENT_ITEM_TYPE) {
        return (EventItem *)qitem;
    }
    return NULL;
}

void EventsListView::onItemDoubleClicked(const QModelIndex& index)
{
    EventItem *item = getItem(index);
    if (!item) {
        return;
    }

    const SeafEvent& event = item->event();

    if (!event.isDetailsDisplayable()) {
        return;
    }

    EventDetailsDialog dialog(event, seafApplet->mainWindow());

    dialog.exec();
}

bool EventsListView::viewportEvent(QEvent *event)
{
    if (event->type() != QEvent::ToolTip && event->type() != QEvent::WhatsThis) {
        return QListView::viewportEvent(event);
    }

    QPoint global_pos = QCursor::pos();
    QPoint viewport_pos = viewport()->mapFromGlobal(global_pos);
    QModelIndex index = indexAt(viewport_pos);
    if (!index.isValid()) {
        return true;
    }

    EventItem *item = getItem(index);
    if (!item) {
        return true;
    }

    QRect item_rect = visualRect(index);

    QString text = "<p style='white-space:pre'>";
    text += item->event().repo_name;
    text += "</p>";

    QToolTip::showText(QCursor::pos(), text, viewport(), item_rect);

    return true;
}


EventsListModel::EventsListModel(QObject *parent)
    : QStandardItemModel(parent)
{
}

const QModelIndex
EventsListModel::updateEvents(const std::vector<SeafEvent>& events, bool is_loading_more)
{
    if (!is_loading_more) {
        clear();
    }
    int i = 0, n = events.size();

    EventItem *first_item = 0;

    for (i = 0; i < n; i++) {
        SeafEvent event = events[i];
        EventItem *item = new EventItem(event);

        appendRow(item);

        if (!first_item) {
            first_item = item;
        }
    }

    if (is_loading_more && first_item) {
        return indexFromItem(first_item);
    }

    return QModelIndex();
}

void EventsListModel::onAvatarUpdated(const QString& email, const QImage& img)
{
    int i, n = rowCount();

    for (i = 0; i < n; i++) {
        QStandardItem *qitem = item(i);
        if (qitem->type() != EVENT_ITEM_TYPE) {
            return;
        }
        EventItem *item = (EventItem *)qitem;

        if (item->event().author == email) {
            QModelIndex index = indexFromItem(item);
            emit dataChanged(index, index);
        }
    }
}
