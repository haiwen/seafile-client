#include "file-delegate.h"
#include "file-table-view.h"
 
#include <QtGui>
#include <QMouseEvent>
//#include <QStandItem>
#include <QDebug>
 
FileTableView::FileTableView(const ServerRepo& repo, QWidget *parent)
    : QTableView(parent),
      currHovered(-1),
      repo_(repo)
{
    horizontalHeader()->setResizeMode(QHeaderView::Stretch);

    connect(this, SIGNAL(doubleClicked(const QModelIndex&)),
            this, SLOT(onItemDoubleClicked(const QModelIndex&)));

    FileDelegate *delegate = new FileDelegate;
    delegate->setView(this);
 
    setItemDelegate(delegate);
    setMouseTracking(true);
}

void FileTableView::onItemDoubleClicked(const QModelIndex& index)
{
    if (index.column() != FILE_COLUMN_NAME)
        return;
    FileTableModel *_model = static_cast<FileTableModel*>(model());
    const SeafDirent dirent = _model->direntAt(index.row());

    emit direntClicked(dirent);
}

void FileTableView::setMouseOver(const int row)
{
    if ( row == currHovered) return;
 
    FileTableModel *_model = static_cast<FileTableModel*>(model());
    for ( int col = 0; col < _model->columnCount(); col++ )
    {
        //setBackground(QBrush(QColor(200, 200, 220, 255)));
    }
 
    if ( currHovered != -1 )
        disableMouseOver();
 
    currHovered = row;
}
 
void FileTableView::disableMouseOver()
{
    QStandardItemModel *_model = static_cast<QStandardItemModel*>(model());
    for ( int col = 0; col < _model->columnCount(); col++ )
    {
        QStandardItem *item = _model->item(currHovered, col);
 
        item->setBackground(QBrush(QColor("white")));
    }
}
 
void FileTableView::mouseMoveEvent(QMouseEvent *event)
{
 
    // TODO: you need know when mouse are not in table rect
    // then you need disable over
 
    QTableView::mouseMoveEvent(event);
}
