#include <QPainter>
#include <QApplication>
#include <QPixmap>

#include "api/server-repo.h"
#include "repo-item.h"
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

const int kRepoIconHeight = 48;
const int kRepoIconWidth = 48;
const int kRepoNameWidth = 200;
const int kRepoNameHeight = 40;
const int kRepoStatusIconWidth = 48;
const int kRepoStatusIconHeight = 48;

const int kMargin1 = 5;
const int kMargin2= 5;

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
    if (option.state & (QStyle::State_HasFocus | QStyle::State_Selected)) {
        backBrush = option.palette.brush(QPalette::Highlight);
        foreColor = option.palette.color(QPalette::HighlightedText);

    } else if (option.state & QStyle::State_MouseOver) {
        backBrush = option.palette.color( QPalette::Highlight ).light(115);
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
        if ( bs > Qt::NoBrush && bs < Qt::TexturePattern )
            opt.backgroundBrush = opt.backgroundBrush.color().light( 115 );
        else
            opt.backgroundBrush = backBrush;
    }
    painter->save();
    style->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter, 0);
    painter->restore();

    QPoint repo_icon_pos(kMarginLeft + kPadding, kMarginTop + kPadding);
    repo_icon_pos += option.rect.topLeft();
    painter->drawPixmap(repo_icon_pos,
                        repo.getPixmap().scaled(kRepoIconWidth, kRepoIconHeight));

    QPoint repo_name_pos = repo_icon_pos + QPoint(kRepoIconWidth, kMargin1);
    QRect repo_name_rect(repo_name_pos, QSize(kRepoNameWidth, kRepoNameHeight));
    painter->save();
    painter->setPen(foreColor);
    painter->drawText(repo_name_rect,
                      Qt::AlignLeft | Qt::AlignVCenter | Qt::TextWordWrap | Qt::TextWrapAnywhere,
                      repo.name, &repo_name_rect);
    painter->restore();
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
