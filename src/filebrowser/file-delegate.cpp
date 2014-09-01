#include "file-delegate.h"
 
void FileDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItemV4 o = option;
    initStyleOption(&o, index);
 
    if ( o.state & QStyle::State_MouseOver )
    {
        view->setMouseOver(index.row());
    }
 
    o.state &= ~QStyle::State_MouseOver;
 
    QStyledItemDelegate::paint(painter, o, index);
}
