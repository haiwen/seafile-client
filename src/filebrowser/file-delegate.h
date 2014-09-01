#ifndef FB_DELEGATE_H
#define FB_DELEGATE_H
 
#include <QStyledItemDelegate>
 
#include "file-iview.h"
 
class FileDelegate : public QStyledItemDelegate {
 
private:
    FileIView *view;
 
public:
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
 
    void setView(FileIView *view) { this->view = view; }
};
 
#endif // FB_DELEGATE_H
