#include <QPainter>
#include <QApplication>
#include <QPixmap>

#include "QtAwesome.h"
#include "seafile-applet.h"
#include "rpc/rpc-client.h"
#include "api/server-repo.h"
#include "repo-item.h"
#include "repo-tree-model.h"
#include "repo-item-delegate.h"

namespace {

/**
            RepoName
   RepoIcon          statusIcon cloud
            subtitle
 */

const int kMarginLeft = 5;
const int kMarginRight = 5;
const int kMarginTop = 5;
const int kMarginBottom = 5;
const int kPadding = 5;

const int kRepoIconHeight = 40;
const int kRepoIconWidth = 40;
const int kRepoNameWidth = 180;
const int kRepoNameHeight = 30;
const int kRepoStatusIconWidth = 48;
const int kRepoStatusIconHeight = 48;

const int kMargin1 = 5;
const int kMargin2= 5;

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
    RepoItem *item = getRepoItem(index);
    if (!item) {
        return QStyledItemDelegate::sizeHint(option, index);
    }

    int width = kMarginLeft + kRepoIconWidth + kMargin1 + kRepoNameWidth + kMargin2 \
        + kRepoStatusIconWidth + kMarginRight + kPadding * 2;

    int height = kRepoIconHeight + kPadding * 2 + kMarginTop + kMarginBottom;

    // qDebug("width = %d, height = %d\n", width, height);

    return QSize(width, height);
}

void RepoItemDelegate::paint(QPainter *painter,
                             const QStyleOptionViewItem& option,
                             const QModelIndex& index) const
{
    RepoItem *item = getRepoItem(index);
    if (!item) {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }
    const ServerRepo& repo = item->repo();
    paintRepoItem(painter, option, repo);
}

void RepoItemDelegate::paintRepoItem(QPainter *painter,
                                     const QStyleOptionViewItem& option,
                                     const ServerRepo& repo) const
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
        backBrush = option.palette.color( QPalette::Highlight ).lighter(115);
        foreColor = option.palette.color( QPalette::HighlightedText );
        hover = true;

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
    style->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter, 0);
    painter->restore();

    // Paint repo icon
    QPoint repo_icon_pos(kMarginLeft + kPadding, kMarginTop + kPadding);
    repo_icon_pos += option.rect.topLeft();
    painter->save();
    painter->drawPixmap(repo_icon_pos,
                        repo.getPixmap().scaled(kRepoIconWidth, kRepoIconHeight));
    painter->restore();

    // Paint repo name
    painter->save();
    QPoint repo_name_pos = repo_icon_pos + QPoint(kRepoIconWidth + kMargin1, 0);
    QRect repo_name_rect(repo_name_pos, QSize(kRepoNameWidth, kRepoNameHeight));
    painter->setPen(foreColor);
    painter->drawText(repo_name_rect,
                      Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap,
                      fitTextToWidth(repo.name, option.font, kRepoNameWidth),
                      &repo_name_rect);

    // Paint repo description
    QPoint repo_desc_pos = repo_name_rect.bottomLeft() + QPoint(0, 5);
    QRect repo_desc_rect(repo_desc_pos, QSize(kRepoNameWidth, kRepoNameHeight));
    painter->setPen(selected ? foreColor.darker(115) : foreColor.lighter(150));
    painter->setFont(zoomFont(painter->font(), 0.8));
    painter->drawText(repo_desc_rect,
                      Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap,
                      fitTextToWidth(repo.description, option.font, kRepoNameWidth),
                      &repo_desc_rect);
    painter->restore();

    // if (seafApplet->rpcClient()->hasLocalRepo(repo.id)) {
    //     return;
    // }

    // Paint a cloud for repos not in local
    // painter->save();
    // painter->setFont(awesome->font(24));
    // QRect cloud_icon_rect(option.rect.topRight() + QPoint(-32, 0),
    //                       QSize(32, option.rect.height()));
    // painter->drawText(cloud_icon_rect,
    //                   Qt::AlignCenter,
    //                   QChar(icon_cloud));
    // painter->restore();
}


RepoItem* RepoItemDelegate::getRepoItem(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return NULL;
    }
    const RepoTreeModel *model = (const RepoTreeModel*)index.model();
    QStandardItem *item = model->itemFromIndex(index);
    if (item->type() != REPO_ITEM_TYPE) {
        return NULL;
    }
    return (RepoItem *)item;
}
