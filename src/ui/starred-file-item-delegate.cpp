#include <QPainter>
#include <QApplication>
#include <QPixmap>

#include "utils/utils.h"
#include "utils/paint-utils.h"
#include "utils/file-utils.h"
#include "seafile-applet.h"
#include "main-window.h"
#include "rpc/rpc-client.h"
#include "starred-files-list-model.h"
#include "starred-file-item.h"

#include "starred-file-item-delegate.h"

namespace {

/**
            name
   icon
            subtitle
 */

const int kMarginLeft = 5;
const int kMarginRight = 5;
const int kMarginTop = 5;
const int kMarginBottom = 5;
const int kPadding = 5;

const int kFileIconHeight = 36;
const int kFileIconWidth = 36;
const int kFileNameWidth = 210;
const int kFileNameHeight = 30;

const int kMarginBetweenFileIconAndName = 10;

const char *kFileNameColor = "#3F3F3F";
const char *kFileNameColorHighlighted = "#544D49";
const char *kSubtitleColor = "#959595";
const char *kSubtitleColorHighlighted = "#9D9B9A";
const int kFileNameFontSize = 14;
const int kSubtitleFontSize = 13;

const char *kFileItemBackgroundColor = "white";
const char *kFileItemBackgroundColorHighlighted = "#F9E0C7";

const char *kItemBottomBorderColor = "#EEE";

} // namespace

StarredFileItemDelegate::StarredFileItemDelegate(QObject *parent)
  : QStyledItemDelegate(parent)
{
}

QSize StarredFileItemDelegate::sizeHint(const QStyleOptionViewItem &option,
                                        const QModelIndex &index) const
{
    StarredFileItem *item = getItem(index);
    if (!item) {
        return QStyledItemDelegate::sizeHint(option, index);
    }

    return sizeHintForItem(option, item);
}

QSize StarredFileItemDelegate::sizeHintForItem(const QStyleOptionViewItem &option,
                                               const StarredFileItem *item) const
{
    static const int width = kMarginLeft + kFileIconWidth
        + kMarginBetweenFileIconAndName + kFileNameWidth
        + kMarginRight + kPadding * 2;

    static const int height = kFileIconHeight + kPadding * 2 + kMarginTop + kMarginBottom;

    return QSize(width, height);
}

void StarredFileItemDelegate::paint(QPainter *painter,
                                    const QStyleOptionViewItem& option,
                                    const QModelIndex& index) const
{
    StarredFileItem *item = getItem(index);

    paintItem(painter, option, item);
}

void StarredFileItemDelegate::paintItem(QPainter *painter,
                                        const QStyleOptionViewItem& option,
                                        const StarredFileItem *item) const
{
    QBrush backBrush;
    bool selected = false;
    const StarredFile& file = item->file();

    if (option.state & (QStyle::State_HasFocus | QStyle::State_Selected)) {
        backBrush = QColor(kFileItemBackgroundColorHighlighted);
        selected = true;

    } else {
        backBrush = QColor(kFileItemBackgroundColor);
    }

    painter->save();
    painter->fillRect(option.rect, backBrush);
    painter->restore();

    // paint file icon
    QPixmap icon = getIconForFile(file.name());
    QPoint file_icon_pos(kMarginLeft + kPadding, kMarginTop + kPadding);
    file_icon_pos += option.rect.topLeft();
    painter->save();
    painter->drawPixmap(file_icon_pos, icon);
    painter->restore();

    // Calculate the file column by the delta of mainwindow's width
    const int file_name_width = kFileNameWidth
      + seafApplet->mainWindow()->width() - seafApplet->mainWindow()->minimumWidth();

    // Paint file name
    painter->save();
    QPoint file_name_pos = file_icon_pos + QPoint(kFileIconWidth + kMarginBetweenFileIconAndName, 0);
    QRect file_name_rect(file_name_pos, QSize(file_name_width, kFileNameHeight));
    painter->setPen(QColor(selected ? kFileNameColorHighlighted : kFileNameColor));
    painter->setFont(changeFontSize(painter->font(), kFileNameFontSize));

    painter->drawText(file_name_rect,
                      Qt::AlignLeft | Qt::AlignTop,
                      fitTextToWidth(file.name(), option.font, file_name_width),
                      &file_name_rect);
    painter->restore();

    // Paint subtitle
    QString subtitle, size, mtime;

    size = readableFileSize(file.size);
    mtime = translateCommitTime(file.mtime);

    subtitle = size + "  " + mtime;

    painter->save();
    QPoint file_desc_pos = file_name_rect.bottomLeft() + QPoint(0, 5);
    QRect file_desc_rect(file_desc_pos, QSize(file_name_width, kFileNameHeight));
    painter->setFont(changeFontSize(painter->font(), kSubtitleFontSize));
    painter->setPen(QColor(selected ? kSubtitleColorHighlighted : kSubtitleColor));
    painter->drawText(file_desc_rect,
                      Qt::AlignLeft | Qt::AlignTop,
                      fitTextToWidth(subtitle, option.font, file_name_width),
                      &file_desc_rect);
    painter->restore();

    // Draw the bottom border lines
    painter->save();
    painter->setPen(QPen(QColor(kItemBottomBorderColor), 1, Qt::SolidLine));
    painter->drawLine(option.rect.bottomLeft(), option.rect.bottomRight());
    painter->restore();
}

QPixmap StarredFileItemDelegate::getIconForFile(const QString& name) const
{
    return QIcon(::getIconByFileName(name)).pixmap(QSize(kFileIconWidth, kFileIconHeight));
}

StarredFileItem* StarredFileItemDelegate::getItem(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return NULL;
    }

    const StarredFilesListModel *model = (const StarredFilesListModel*)index.model();

    return (StarredFileItem *)(model->itemFromIndex(index));
}
