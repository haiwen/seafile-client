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
         nick         date
   icon
         description  repo name
 */

const int kMarginLeft = 5;
const int kMarginRight = 5;
const int kMarginTop = 5;
const int kMarginBottom = 5;
const int kPadding = 5;
#ifdef Q_OS_MAC
const int kExtraPadding = 5;
#else
const int kExtraPadding = 0;
#endif

const int kAvatarHeight = 40;
const int kAvatarWidth = 40;
//const int kNickWidth = 210;
const int kNickHeight = 30;

const int kMarginBetweenAvatarAndNick = 10;
const int kVerticalMarginBetweenNickAndDesc = 3;

const char *kNickColor = "#D8AC8F";
const char *kNickColorHighlighted = "#D8AC8F";
const char *kRepoNameColor = "#D8AC8F";
const char *kRepoNameColorHighlighted = "#D8AC8F";
const char *kDescriptionColor = "#3F3F3F";
const char *kDescriptionColorHighlighted = "#544D49";

const int kNickFontSize = 16;
const int kDescriptionFontSize = 13;
const int kDescriptionHeight = 30;

const char *kEventItemBackgroundColor = "white";
const char *kEventItemBackgroundColorHighlighted = "#F9E0C7";

const int kTimeWidth = 100;
const int kTimeHeight = 30;
const int kTimeFontSize = 13;

const int kRepoNameWidth = 80;

const char *kTimeColor = "#959595";
const char *kTimeColorHighlighted = "#9D9B9A";

const int kMarginBetweenNickAndTime = 10;

const int kMarginBetweenRepoNameAndDesc = 10;


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
    if (!item) {
        return;
    }

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
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setRenderHint(QPainter::HighQualityAntialiasing);

    // get the device pixel radio from current painter device
    double scale_factor = 1;
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    scale_factor = globalDevicePixelRatio();
#endif // QT5

    // paint avatar
    QImage avatar;
    if (!event.anonymous) {
        avatar = AvatarService::instance()->getAvatar(event.author);
    }

    QRect actualRect(0, 0, kAvatarWidth * scale_factor , kAvatarHeight * scale_factor);
    avatar.scaled(actualRect.size());
    QImage masked_image(actualRect.size(), QImage::Format_ARGB32_Premultiplied);
    masked_image.fill(Qt::transparent);
    QPainter mask_painter;
    mask_painter.begin(&masked_image);
    mask_painter.setRenderHint(QPainter::Antialiasing);
    mask_painter.setRenderHint(QPainter::HighQualityAntialiasing);
    mask_painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    mask_painter.setPen(Qt::NoPen);
    mask_painter.setBrush(Qt::white);
    mask_painter.drawEllipse(actualRect);
    mask_painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    mask_painter.drawImage(actualRect, avatar);
    mask_painter.setCompositionMode(QPainter::CompositionMode_DestinationOver);
    mask_painter.fillRect(actualRect, Qt::transparent);
    mask_painter.end();
    masked_image.setDevicePixelRatio(scale_factor);

    QPoint avatar_pos(kMarginLeft + kPadding, kMarginTop + kPadding);
    avatar_pos += option.rect.topLeft();
    painter->save();
    painter->drawImage(avatar_pos, masked_image);
    painter->restore();

    auto time_font = changeFontSize(painter->font(), kTimeFontSize);
    auto nick_font = changeFontSize(painter->font(), kNickFontSize);
    auto desc_font = changeFontSize(painter->font(), kDescriptionFontSize);
    auto repo_name_font = time_font;

    const int time_width = qMin(kTimeWidth, ::textWidthInFont(time_text, time_font));
    int nick_width = option.rect.width() - kMarginLeft - kAvatarWidth - kMarginBetweenAvatarAndNick
        - time_width - kMarginBetweenNickAndTime - kPadding * 2 - kMarginRight;
    nick_width = qMin(nick_width, ::textWidthInFont(event.nick, nick_font));

    // Paint nick name
    QPoint nick_pos = avatar_pos + QPoint(kAvatarWidth + kMarginBetweenAvatarAndNick, 0);
    QRect nick_rect(nick_pos, QSize(nick_width, kNickHeight));
    painter->save();
    painter->setPen(QColor(selected ? kNickColorHighlighted : kNickColor));
    painter->setFont(nick_font);
    painter->drawText(nick_rect,
                      Qt::AlignLeft | Qt::AlignTop,
                      fitTextToWidth(event.nick, option.font, nick_width),
                      &nick_rect);
    painter->restore();

    // Paint event time
    painter->save();
    QPoint time_pos = option.rect.topRight() + QPoint(-time_width - kPadding - kMarginRight, kMarginTop + kPadding);
    QRect time_rect(time_pos, QSize(time_width, kTimeHeight));
    painter->setPen(QColor(selected ? kTimeColorHighlighted : kTimeColor));
    painter->setFont(time_font);

    painter->drawText(time_rect,
                      Qt::AlignRight | Qt::AlignTop,
                      time_text,
                      &time_rect);
    painter->restore();

    // Paint description
    painter->save();

    QString desc = event.desc;

    int repo_name_width = qMin(kRepoNameWidth, ::textWidthInFont(event.repo_name, repo_name_font));

    int desc_width = option.rect.width() - kMarginLeft - kAvatarWidth -
                     kMarginBetweenAvatarAndNick -
                     kMarginBetweenRepoNameAndDesc - repo_name_width -
                     kMarginRight - kPadding * 2;

    const int desc_height = ::textHeightInFont(desc, desc_font) * 2;

    // const QPoint event_desc_pos = option.rect.bottomLeft() + QPoint(nick_rect.left(), - desc_height - kExtraPadding - kMarginBottom);
    const QPoint event_desc_pos = nick_rect.bottomLeft() + QPoint(0, kVerticalMarginBetweenNickAndDesc);

    QRect event_desc_rect(event_desc_pos, QSize(desc_width, desc_height));
    painter->setFont(desc_font);
    painter->setPen(QColor(selected ? kDescriptionColorHighlighted : kDescriptionColor));

    desc.replace(QChar('\n'), QChar(' '));
    painter->drawText(event_desc_rect,
                      Qt::AlignLeft | Qt::AlignTop | Qt::TextWrapAnywhere,
                      // we have two lines
                      fitTextToWidth(desc, desc_font, desc_width * 2),
                      &event_desc_rect);
    painter->restore();

    // Paint repo name
    painter->save();

    repo_name_width += desc_width - event_desc_rect.width();
    // if (index.row() == 1) {
    //     printf ("width diff = %d\n", desc_width - event_desc_rect.width());
    // }

    const int repo_name_height = ::textHeightInFont(event.repo_name, repo_name_font);
    const QPoint event_repo_name_pos = option.rect.bottomRight() +
        QPoint(-repo_name_width - kPadding - kMarginRight,
               -repo_name_height - kExtraPadding - kMarginBottom);

    QRect event_repo_name_rect(event_repo_name_pos, QSize(repo_name_width, kNickHeight));
    painter->setFont(repo_name_font);
    painter->setPen(QColor(selected ? kRepoNameColorHighlighted : kRepoNameColor));
    painter->drawText(
        event_repo_name_rect,
        Qt::AlignRight | Qt::AlignTop | Qt::TextSingleLine,
        fitTextToWidth(event.repo_name, repo_name_font, repo_name_width),
        &event_repo_name_rect);
    painter->restore();

    // const EventsListModel *model = (const EventsListModel*)index.model();
    //
    // Draw the bottom border lines except for the last item (if the "load more" is present")
    // We minus 2 here becase:
    // 1) the row index starts from 0
    // 2) we addded a row for the "load more" data in the end
    // if (model->loadMoreIndex().isValid() && index.row() == model->rowCount() - 2) {
    //     return;
    // }

    painter->save();
    painter->setPen(QPen(QColor(kItemBottomBorderColor), 1, Qt::SolidLine));
    QPoint left = option.rect.bottomLeft();
    left.setY(left.y() + 1);
    QPoint right = option.rect.bottomRight();
    right.setY(right.y() + 1);
    painter->drawLine(left, right);
    painter->restore();
}

QSize EventItemDelegate::sizeHint(const QStyleOptionViewItem& option,
                                  const QModelIndex& index) const
{
    int height = kNickHeight + kDescriptionHeight + kPadding * 4 - kExtraPadding;
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
    } else {
        return NULL;
    }
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

    EventDetailsDialog* dialog = new EventDetailsDialog(event, seafApplet->mainWindow());
    dialog->setAttribute(Qt::WA_DeleteOnClose, true);
    dialog->show();
    dialog->raise();
    dialog->activateWindow();
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
    text += item->event().desc;
    text += "</p>";

    QToolTip::showText(QCursor::pos(), text, viewport(), item_rect);

    return true;
}


EventsListModel::EventsListModel(QObject *parent)
    : QStandardItemModel(parent)
{
}

const QModelIndex
EventsListModel::updateEvents(const std::vector<SeafEvent>& events,
                              bool is_loading_more,
                              bool has_more)
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

    if (has_more) {
        QStandardItem *load_more_item = new QStandardItem();
        appendRow(load_more_item);
        load_more_index_ = load_more_item->index();
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
