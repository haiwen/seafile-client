#include "ui/search-tab.h"
#include "ui/search-tab-items.h"
#include "ui/main-window.h"
#include "utils/utils.h"
#include "utils/paint-utils.h"
#include "seafile-applet.h"
#include "api/requests.h"

#include <QtGlobal>

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
#include <QtWidgets>
#else
#include <QtGui>
#endif

namespace {
const int kMarginLeft = 5;
const int kMarginRight = 5;
const int kMarginTop = 5;
const int kMarginBottom = 5;
const int kPadding = 5;

const int kFileIconHeight = 36;
const int kFileIconWidth = 36;
const int kFileNameWidth = 210;
const int kFileNameHeight = 30;

const int kSubtitleHeight = 16;

const int kMarginBetweenFileIconAndName = 10;

const char *kFileNameColor = "#3F3F3F";
const char *kFileNameColorHighlighted = "#544D49";
const char *kSubtitleColor = "#959595";
const char *kSubtitleColorHighlighted = "#9D9B9A";
const int kFileNameFontSize = 14;
const int kSubtitleFontSize = 11;

const char *kFileItemBackgroundColor = "white";
const char *kFileItemBackgroundColorHighlighted = "#F9E0C7";

const char *kItemBottomBorderColor = "#EEE";

static inline const QListWidgetItem *getItem(const QModelIndex &index)
{
    const SearchResultListModel *model = static_cast<const SearchResultListModel*>(index.model());
    return model->item(index);
}
static inline FileSearchResult getSearchResult(const QModelIndex &index)
{
    const QListWidgetItem *item = getItem(index);
    if (!item)
        return FileSearchResult();
    return item->data(Qt::UserRole).value<FileSearchResult>();
}
} // anonymous namespace

void SearchResultItemDelegate::paint(QPainter *painter,
                                     const QStyleOptionViewItem &option,
                                     const QModelIndex &index) const {
    const SearchResultListModel *model = static_cast<const SearchResultListModel*>(index.model());
    QBrush backBrush;
    bool selected = false;
    FileSearchResult file = getSearchResult(index);

    if (option.state & (QStyle::State_HasFocus | QStyle::State_Selected)) {
        backBrush = QColor(kFileItemBackgroundColorHighlighted);
        selected = true;
    } else {
        backBrush = QColor(kFileItemBackgroundColor);
    }

    //
    // draw item's background
    //
    painter->save();
    painter->fillRect(option.rect, backBrush);
    painter->restore();

    QIcon icon = model->data(index, Qt::DecorationRole).value<QIcon>();
    // get the device pixel radio from current painter device
    int scale_factor = 1;
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    scale_factor = painter->device()->devicePixelRatio();
#endif // QT5
    QPixmap pixmap(icon.pixmap(QSize(kFileIconWidth, kFileIconHeight) * scale_factor));
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    if (pixmap.size() != QSize(kFileIconWidth, kFileIconHeight))
        pixmap.setDevicePixelRatio(scale_factor);
#endif // QT5

    //
    // paint file icon
    //
    QPoint file_icon_pos(kMarginLeft + kPadding, kMarginTop + kPadding);
    file_icon_pos += option.rect.topLeft();
    painter->save();
    painter->drawPixmap(file_icon_pos, pixmap);
    painter->restore();

    // Calculate the file column by the delta of mainwindow's width
    QString title = file.name;

    const int file_name_width = kFileNameWidth
      + seafApplet->mainWindow()->width() - seafApplet->mainWindow()->minimumWidth();
    painter->save();
    QPoint file_name_pos = file_icon_pos + QPoint(kFileIconWidth + kMarginBetweenFileIconAndName, 0);
    QRect file_name_rect(file_name_pos, QSize(file_name_width, kFileNameHeight));
    painter->setPen(QColor(selected ? kFileNameColorHighlighted : kFileNameColor));
    painter->setFont(changeFontSize(painter->font(), kFileNameFontSize));

    painter->drawText(file_name_rect,
                      Qt::AlignLeft | Qt::AlignTop,
                      fitTextToWidth(title, option.font, file_name_width),
                      &file_name_rect);
    painter->restore();

    //
    // Paint repo_name
    //
    int count_of_splash = file.fullpath.endsWith("/") ? 2 : 1;
    QString subtitle = file.fullpath.mid(1, file.fullpath.size() - count_of_splash - file.name.size());
    if (!subtitle.isEmpty())
        subtitle = file.repo_name + "/" + subtitle.left(subtitle.size() - 1);
    else
        subtitle = file.repo_name;

    painter->save();
    QPoint file_desc_pos = file_name_rect.bottomLeft() + QPoint(0, kPadding / 2);
    QRect file_desc_rect(file_desc_pos, QSize(file_name_width, kSubtitleHeight));
    painter->setFont(changeFontSize(painter->font(), kSubtitleFontSize));
    painter->setPen(QColor(selected ? kSubtitleColorHighlighted : kSubtitleColor));
    painter->drawText(file_desc_rect,
                      Qt::AlignLeft | Qt::AlignTop,
                      fitTextToWidth(subtitle, option.font, file_name_width),
                      &file_desc_rect);
    painter->restore();

    //
    // Paint file description
    //
    QString size, mtime;

    size = readableFileSize(file.size);
    mtime = translateCommitTime(file.last_modified);

    QString extra_title = size + "  " + mtime;

    painter->save();
    QPoint file_extra_pos = file_desc_rect.bottomLeft() + QPoint(0, kPadding / 2 + 2);
    QRect file_extra_rect(file_extra_pos, QSize(file_name_width, kSubtitleHeight));
    painter->setFont(changeFontSize(painter->font(), kSubtitleFontSize));
    painter->setPen(QColor(selected ? kSubtitleColorHighlighted : kSubtitleColor));
    painter->drawText(file_extra_rect,
                      Qt::AlignLeft | Qt::AlignTop,
                      fitTextToWidth(extra_title, option.font, file_name_width),
                      &file_extra_rect);
    painter->restore();

    //
    // Draw the bottom border lines
    //
    painter->save();
    painter->setPen(QPen(QColor(kItemBottomBorderColor), 1, Qt::SolidLine));
    painter->drawLine(option.rect.bottomLeft(), option.rect.bottomRight());
    painter->restore();
}

QSize SearchResultItemDelegate::sizeHint(const QStyleOptionViewItem &option,
                                         const QModelIndex &index) const {

    const QListWidgetItem* item = getItem(index);
    if (!item)
        return QStyledItemDelegate::sizeHint(option, index);

    return QSize(kMarginLeft + kFileIconWidth + kMarginBetweenFileIconAndName + kFileNameWidth + kMarginRight + kPadding * 2,
                 kFileIconHeight + kPadding * 3 + kMarginTop + kMarginBottom);
}
