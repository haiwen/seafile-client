#include "file-table-view.h"

#include <QtGui>
#include <QMouseEvent>

#include "file-delegate.h"

FileTableView::FileTableView(QWidget *parent)
    : QTableView(parent)
{
    verticalHeader()->hide();
    verticalHeader()->setDefaultSectionSize(40);
    horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
    horizontalHeader()->setStretchLastSection(true);
    horizontalHeader()->setCascadingSectionResizes(true);
    horizontalHeader()->setHighlightSections(false);
    horizontalHeader()->setSortIndicatorShown(false);
    horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setAlternatingRowColors(true);

    setGridStyle(Qt::NoPen);
    setShowGrid(false);

    setContentsMargins(0, 0, 0, 0);
    setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

    connect(this, SIGNAL(doubleClicked(const QModelIndex&)),
            this, SLOT(onItemDoubleClicked(const QModelIndex&)));

    FileDelegate *delegate = new FileDelegate;
    delegate->setView(this);
    setItemDelegate(delegate);
    setMouseTracking(true);

    setAcceptDrops(true);
    setDragDropMode(QAbstractItemView::DropOnly);
}

void FileTableView::setMouseOver(const int row)
{
    Q_UNUSED(row);
    //disable hover effect temporary
    /*
    FileTableModel *_model = static_cast<FileTableModel*>(model());

    _model->setMouseOver(row);
    */
}

void FileTableView::setModel(QAbstractItemModel *p_model)
{
    if (p_model == NULL || p_model == model()) //if model is not changed
        return;

    QTableView::setModel(p_model);

    FileTableModel *model_ = static_cast<FileTableModel*>(p_model);

    connect(this, SIGNAL(direntClicked(const SeafDirent&)),
            model_, SLOT(onEnter(const SeafDirent&)));

    connect(this, SIGNAL(dropFile(const QString &)),
            model_, SLOT(onFileUpload(const QString &)));

    connect(this, SIGNAL(selectionChanged(const int)),
            model_, SLOT(onSelectionChanged(const int)));
}

void FileTableView::onItemDoubleClicked(const QModelIndex& index)
{
    FileTableModel *_model = static_cast<FileTableModel*>(model());
    const SeafDirent dirent = _model->direntAt(index.row());

    emit direntClicked(dirent);
}

void FileTableView::leaveEvent(QEvent *event)
{
    /*
    if (event->type() == QEvent::Leave) {
        FileTableModel *_model = static_cast<FileTableModel*>(model());
        _model->setMouseOver(-1);
    }
    */
    QTableView::leaveEvent(event);
}

void FileTableView::dropEvent(QDropEvent *event)
{
    QList<QUrl> urls = event->mimeData()->urls();

    if(urls.isEmpty())
        return;

    foreach(const QUrl &url, urls)
    {
        QString file_name = url.toLocalFile();

        if(file_name.isEmpty())
            continue;

        emit dropFile(file_name);
    }

    event->accept();
}

void FileTableView::dragMoveEvent(QDragMoveEvent *event)
{
    //only handle external source currently
    if(event->source() != NULL)
        return;

    event->accept();
}

void FileTableView::dragEnterEvent(QDragEnterEvent *event)
{
    //only handle external source currently
    if(event->source() != NULL)
        return;

    event->setDropAction(Qt::CopyAction);

    if(event->mimeData()->hasFormat("text/uri-list"))
        event->accept();
}


void FileTableView::selectionChanged(const QItemSelection &selected,
                                     const QItemSelection &deselected)
{
    int row = -1;
    if (&selected == &deselected)
        return;
    if  (!selected.isEmpty())
        row = selected.indexes().first().row();
    emit selectionChanged(row);
}

void FileTableView::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Return ||
        event->key() == Qt::Key_Enter)
        onItemDoubleClicked(currentIndex());
    QAbstractItemView::keyPressEvent(event);
}

void FileTableView::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton)
        return;
    QAbstractItemView::mouseDoubleClickEvent(event);
}

QStyleOptionViewItem FileTableView::viewOptions () const
{
    QStyleOptionViewItemV4 option = QTableView::viewOptions();
    option.displayAlignment = Qt::AlignLeft | Qt::AlignVCenter;
    option.decorationAlignment = Qt::AlignHCenter | Qt::AlignCenter;
    option.decorationPosition = QStyleOptionViewItem::Top;
    return option;
}

