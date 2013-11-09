#include <QPainter>
#include <QApplication>
#include <QPixmap>
#include <QToolTip>

#include "QtAwesome.h"
#include "utils/utils.h"
#include "seafile-applet.h"
#include "rpc/rpc-client.h"
#include "api/server-repo.h"
#include "repo-item.h"
#include "repo-tree-view.h"
#include "repo-tree-model.h"
#include "repo-item-delegate.h"

namespace {

/**
            RepoName
   RepoIcon          statusIcon
            subtitle
 */

const int kMarginLeft = 5;
const int kMarginRight = 5;
const int kMarginTop = 5;
const int kMarginBottom = 5;
const int kPadding = 5;

const int kRepoIconHeight = 36;
const int kRepoIconWidth = 36;
const int kRepoNameWidth = 180;
const int kRepoNameHeight = 30;
const int kRepoStatusIconWidth = 24;
const int kRepoStatusIconHeight = 24;

const int kRepoCategoryNameMaxWidth = 400;
const int kRepoCategoryIndicatorWidth = 16;
const int kRepoCategoryIndicatorHeight = 16;
const int kMarginBetweenIndicatorAndName = 5;

const int kMarginBetweenRepoIconAndName = 5;
const int kMarginBetweenRepoNameAndStatus = 5;



QString fitTextToWidth(const QString& text, const QFont& font, int width)
{
    static QString ELLIPSISES = "...";

	QFontMetrics qfm(font);
	QSize size = qfm.size(0, text);
	if (size.width() <= width)
		return text;				// it fits, so just display it

	// doesn't fit, so we need to truncate and add ellipses
	QSize sizeElippses = qfm.size(0, ELLIPSISES); // we need to cut short enough to add these
	QString s = text;
	while (s.length() > 20)     // never cut shorter than this...
	{
		int len = s.length();
		s = text.left(len-1);
		size = qfm.size(0, s);
		if (size.width() <= (width - sizeElippses.width()))
			break;              // we are finally short enough
	}

	return (s + ELLIPSISES);
}

QFont zoomFont(const QFont& font_in, double ratio)
{
    QFont font(font_in);

    if (font.pointSize() > 0) {
        font.setPointSize((int)(font.pointSize() * ratio));
    } else {
        font.setPixelSize((int)(font.pixelSize() * ratio));
    }

    return font;
}


} // namespace

RepoItemDelegate::RepoItemDelegate(QObject *parent)
  : QStyledItemDelegate(parent)
{
}

QSize RepoItemDelegate::sizeHint(const QStyleOptionViewItem &option,
                                 const QModelIndex &index) const
{
    QStandardItem *item = getItem(index);
    if (!item) {
        return QStyledItemDelegate::sizeHint(option, index);
    }

    if (item->type() == REPO_ITEM_TYPE) {
        return sizeHintForRepoItem(option, (const RepoItem *)item);
    } else {
        // return QStyledItemDelegate::sizeHint(option, index);
        return sizeHintForRepoCategoryItem(option, (const RepoCategoryItem *)item);
    }
}

QSize RepoItemDelegate::sizeHintForRepoItem(const QStyleOptionViewItem &option,
                                            const RepoItem *item) const
{

    int width = kMarginLeft + kRepoIconWidth
        + kMarginBetweenRepoIconAndName + kRepoNameWidth
        + kMarginBetweenRepoNameAndStatus + kRepoStatusIconWidth;
        + kMarginRight + kPadding * 2;

    int height = kRepoIconHeight + kPadding * 2 + kMarginTop + kMarginBottom;

    // qDebug("width = %d, height = %d\n", width, height);

    return QSize(width, height);
}

QSize RepoItemDelegate::sizeHintForRepoCategoryItem(const QStyleOptionViewItem &option,
                                                    const RepoCategoryItem *item) const
{
    int width, height;

	QFontMetrics qfm(option.font);
    QSize size = qfm.size(0, item->name());

    width = qMin(size.width(), kRepoCategoryIndicatorWidth)
        + kMarginBetweenIndicatorAndName + kRepoCategoryNameMaxWidth;

    height = qMax(size.height(), kRepoCategoryIndicatorHeight) + kPadding;

    // qDebug("width = %d, height = %d\n", width, height);

    return QSize(width, height);
}


void RepoItemDelegate::paint(QPainter *painter,
                             const QStyleOptionViewItem& option,
                             const QModelIndex& index) const
{
    QStandardItem *item = getItem(index);
    if (!item) {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    if (item->type() == REPO_ITEM_TYPE) {
        paintRepoItem(painter, option, (RepoItem *)item);
    } else {
        // QStyledItemDelegate::paint(painter, option, index);
        paintRepoCategoryItem(painter, option, (RepoCategoryItem *)item);
    }
}

void RepoItemDelegate::paintRepoItem(QPainter *painter,
                                     const QStyleOptionViewItem& option,
                                     const RepoItem *item) const
{
    const ServerRepo& repo = item->repo();
    QBrush backBrush;
    QColor foreColor;
    bool hover = false;
    bool selected = false;
    if (option.state & (QStyle::State_HasFocus | QStyle::State_Selected)) {
        backBrush = option.palette.brush(QPalette::Highlight);
        backBrush = QColor("#f9e0c7");
        foreColor = option.palette.color( QPalette::Text );
        selected = true;

    } else if (option.state & QStyle::State_MouseOver) {
        // backBrush = option.palette.color( QPalette::Highlight ).lighter(115);
        // foreColor = option.palette.color( QPalette::HighlightedText );
        // hover = true;

    } else {
        backBrush = option.palette.brush( QPalette::Base );
        foreColor = option.palette.color( QPalette::Text );
    }

    QStyle *style = QApplication::style();
    QStyleOptionViewItemV4 opt(option);
    if (hover)
    {
        Qt::BrushStyle bs = opt.backgroundBrush.style();
        if (bs > Qt::NoBrush && bs < Qt::TexturePattern)
            opt.backgroundBrush = opt.backgroundBrush.color().lighter(115);
        else
            opt.backgroundBrush = backBrush;
    }
    painter->save();
    painter->fillRect(option.rect, backBrush);
    painter->restore();

    // Paint repo icon
    QPoint repo_icon_pos(kMarginLeft + kPadding, kMarginTop + kPadding);
    repo_icon_pos += option.rect.topLeft();
    painter->save();
    painter->drawPixmap(repo_icon_pos,
                        repo.getPixmap());
    painter->restore();

    // Paint repo name
    painter->save();
    QPoint repo_name_pos = repo_icon_pos + QPoint(kRepoIconWidth + kMarginBetweenRepoIconAndName, 0);
    QRect repo_name_rect(repo_name_pos, QSize(kRepoNameWidth, kRepoNameHeight));
    painter->setPen(foreColor);
    painter->drawText(repo_name_rect,
                      Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap,
                      fitTextToWidth(repo.name, option.font, kRepoNameWidth),
                      &repo_name_rect);

    // Paint repo description
    QPoint repo_desc_pos = repo_name_rect.bottomLeft() + QPoint(0, 5);
    QRect repo_desc_rect(repo_desc_pos, QSize(kRepoNameWidth, kRepoNameHeight));
    painter->setPen(selected ? foreColor : foreColor.lighter(150));
    painter->setFont(zoomFont(painter->font(), 0.8));

    QString description = item->cloneProgress();
    if (description == "" || description == "done") {
        description = translateCommitTime(repo.mtime);
    }

    painter->drawText(repo_desc_rect,
                      Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap,
                      fitTextToWidth(description, option.font, kRepoNameWidth),
                      &repo_desc_rect);
    painter->restore();

    // Paint repo status icon
    QPoint status_icon_pos = option.rect.topRight() - QPoint(50, 0);
    status_icon_pos.setY(option.rect.center().y() - (kRepoStatusIconHeight / 2));
    QRect status_icon_rect(status_icon_pos, QSize(kRepoStatusIconWidth, kRepoStatusIconHeight));
    if (!item->localRepo().isValid()
        || item->localRepo().sync_state != LocalRepo::SYNC_STATE_WAITING) {

        painter->save();
        painter->drawPixmap(status_icon_pos, getSyncStatusIcon(item));
        painter->restore();
    }

    // Update the metrics of this item
    RepoItem::Metrics metrics;
    QPoint shift(-option.rect.topLeft().x(), -option.rect.topLeft().y());
    metrics.icon_rect = QRect(repo_icon_pos, QSize(kRepoIconWidth, kRepoIconHeight));
    metrics.name_rect = repo_name_rect;
    metrics.subtitle_rect = repo_desc_rect;
    metrics.status_icon_rect = status_icon_rect;

    metrics.icon_rect.translate(shift);
    metrics.name_rect.translate(shift);
    metrics.subtitle_rect.translate(shift);
    metrics.status_icon_rect.translate(shift);

    item->setMetrics(metrics);
}

void RepoItemDelegate::paintRepoCategoryItem(QPainter *painter,
                                             const QStyleOptionViewItem& option,
                                             const RepoCategoryItem *item) const
{
    QBrush backBrush;
    QColor foreColor;
    bool hover = false;
    bool selected = false;

    if (option.state & (QStyle::State_HasFocus | QStyle::State_Selected)) {
        backBrush = option.palette.brush(QPalette::Highlight);
        foreColor = option.palette.color(QPalette::HighlightedText);
        selected = true;

    } else if (option.state & QStyle::State_MouseOver) {
        backBrush = option.palette.color(QPalette::Highlight).lighter(115);
        foreColor = option.palette.color(QPalette::HighlightedText);
        hover = true;

    } else {
        backBrush = option.palette.brush(QPalette::Base);
        foreColor = option.palette.color(QPalette::Text);
    }

    QStyle *style = QApplication::style();
    QStyleOptionViewItemV4 opt(option);
    opt.backgroundBrush = backBrush;
    painter->save();
    style->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter, 0);
    painter->restore();

    // Paint the expand/collapse indicator
    RepoTreeModel *model = (RepoTreeModel *)item->model();
    RepoTreeView *view = model->treeView();
    bool expanded = view->isExpanded(model->indexFromItem(item));

    QRect indicator_rect(option.rect.topLeft(),
                         option.rect.bottomLeft() + QPoint(option.rect.height(), 0));
    painter->save();
    painter->setPen(foreColor);
    painter->setFont(awesome->font(16));
    painter->drawText(indicator_rect,
                      Qt::AlignCenter,
                      QChar(expanded ? icon_caret_down : icon_caret_right),
                      &indicator_rect);
    painter->restore();

    // Paint category name
    painter->save();
    QPoint category_name_pos = indicator_rect.topRight() + QPoint(kMarginBetweenIndicatorAndName, 0);
    QRect category_name_rect(category_name_pos,
                             option.rect.bottomRight() - QPoint(kPadding, 0));
    painter->setPen(foreColor);
    painter->drawText(category_name_rect,
                      Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap,
                      fitTextToWidth(item->name() + QString().sprintf(" [%d]", item->rowCount()),
                                     option.font, category_name_rect.width()));
    painter->restore();
}

QPixmap RepoItemDelegate::getSyncStatusIcon(const RepoItem *item) const
{
    const QString prefix = ":/images/sync/";
    const LocalRepo& repo = item->localRepo();
    QString icon;
    if (!repo.isValid()) {
        icon = "cloud";
    } else {
        switch (repo.sync_state) {
        case LocalRepo::SYNC_STATE_DONE:
            icon = "ok";
            break;
        case LocalRepo::SYNC_STATE_ING:
            icon = "refresh";
            break;
        case LocalRepo::SYNC_STATE_ERROR:
            icon = "exclamation";
            break;
        case LocalRepo::SYNC_STATE_WAITING:
            icon = "minus-sign";
            break;
        case LocalRepo::SYNC_STATE_DISABLED:
            icon = "pause";
            break;
        case LocalRepo::SYNC_STATE_UNKNOWN:
            icon = "question";
            break;
        }
    }

    return prefix + icon + ".png";
}

QStandardItem* RepoItemDelegate::getItem(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return NULL;
    }
    const RepoTreeModel *model = (const RepoTreeModel*)index.model();
    QStandardItem *item = model->itemFromIndex(index);
    if (item->type() != REPO_ITEM_TYPE &&
        item->type() != REPO_CATEGORY_TYPE) {
        return NULL;
    }
    return item;
}

void RepoItemDelegate::showRepoItemToolTip(const RepoItem *item,
                                           const QPoint& global_pos,
                                           QWidget *viewport,
                                           const QRect& rect) const
{
    const RepoItem::Metrics& metrics = item->metrics();

    const QRect& status_icon_rect = metrics.status_icon_rect;

    QPoint viewpos = viewport->mapFromGlobal(global_pos);
    viewpos -= rect.topLeft();

    // QRect r(status_icon_rect);
    // qDebug("rect: (%d, %d) w = %d, h = %d; pos: (%d, %d)\n",
    //        r.topLeft().x(), r.topLeft().y(), r.width(), r.height(), viewpos.x(), viewpos.y());

    if (!status_icon_rect.contains(viewpos)) {
        return;
    }

    QString text = "<p style='white-space:pre'>";
    const LocalRepo& local_repo = item->localRepo();
    if (!local_repo.isValid()) {
        text += tr("This library has not been downloaded");
    } else {
        if (local_repo.sync_state == LocalRepo::SYNC_STATE_ERROR) {
            text += local_repo.sync_error_str;
        } else {
            text += local_repo.sync_state_str;
        }
    }
    text += "</p>";

    QPoint tool_tip_pos = viewport->mapToGlobal(status_icon_rect.center()
                                                + rect.topLeft());
    QToolTip::showText(tool_tip_pos, text,
                       viewport,
                       status_icon_rect.translated(rect.topLeft()));
}
