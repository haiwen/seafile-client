#ifndef FB_TABLE_VIEW_H
#define FB_TABLE_VIEW_H

#include <QTableView>
#include "file-table.h"
#include "file-iview.h"

class FileTableView : public QTableView, public FileIView
{
    Q_OBJECT
public:
    FileTableView(const ServerRepo& repo, QWidget *parent=0);
    void setMouseOver(const int row);

signals:
    void direntClicked(const SeafDirent& dirent);
    void dropFile(const QString &file_name);
    void selectionChanged(const int row);

private slots:
    void onItemDoubleClicked(const QModelIndex& index);

private:
    Q_DISABLE_COPY(FileTableView)
    void leaveEvent(QEvent *event);

    void dropEvent(QDropEvent *event);
    void dragEnterEvent(QDragEnterEvent *event);
    void dragMoveEvent(QDragMoveEvent *event);

    void selectionChanged(const QItemSelection &selected,
                          const QItemSelection &deselected);

    void keyPressEvent(QKeyEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);

    QStyleOptionViewItem viewOptions() const;

    ServerRepo repo_;
};



#endif
