#ifndef SEAFILE_CLIENT_REPO_ITEM_DELEGATE_H
#define SEAFILE_CLIENT_REPO_ITEM_DELEGATE_H

#include <QStyledItemDelegate>
#include <QPixmap>

class QStandardItem;
class QModelIndex;
class QWidget;

class StarredFileItem;

class StarredFileItemDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit StarredFileItemDelegate(QObject *parent=0);

    void paint(QPainter *painter,
               const QStyleOptionViewItem& option,
               const QModelIndex& index) const;

    QSize sizeHint(const QStyleOptionViewItem& option,
                   const QModelIndex& index) const;


private:
    void paintItem(QPainter *painter,
                   const QStyleOptionViewItem& opt,
                   const StarredFileItem *item) const;

    QSize sizeHintForItem(const QStyleOptionViewItem &option,
                          const StarredFileItem *item) const;

    StarredFileItem* getItem(const QModelIndex &index) const;

    QPixmap getIconForFile(const QString& name) const;
};


#endif // SEAFILE_CLIENT_REPO_ITEM_DELEGATE_H
