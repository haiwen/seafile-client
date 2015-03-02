#ifndef SEAFILE_CLIENT_REPO_ITEM_DELEGATE_H
#define SEAFILE_CLIENT_REPO_ITEM_DELEGATE_H

#include <QStyledItemDelegate>
#include <QHash>

class QStandardItem;
class QModelIndex;
class QWidget;

class ServerRepo;
class RepoItem;
class RepoCategoryItem;

class RepoItemDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit RepoItemDelegate(QObject *parent=0);

    void paint(QPainter *painter,
               const QStyleOptionViewItem& option,
               const QModelIndex& index) const;

    QSize sizeHint(const QStyleOptionViewItem& option,
                   const QModelIndex& index) const;

    void showRepoItemToolTip(const RepoItem *item,
                             const QPoint& global_pos,
                             QWidget *viewport,
                             const QRect& rect) const;

private:
    QStandardItem* getItem(const QModelIndex &index) const;
    void paintRepoItem(QPainter *painter,
                       const QStyleOptionViewItem& opt,
                       const RepoItem *item) const;

    void paintRepoCategoryItem(QPainter *painter,
                               const QStyleOptionViewItem& opt,
                               const RepoCategoryItem *item) const;

    QSize sizeHintForRepoCategoryItem(const QStyleOptionViewItem &option,
                                      const RepoCategoryItem *item) const;

    QSize sizeHintForRepoItem(const QStyleOptionViewItem &option,
                              const RepoItem *item) const;

    QIcon getSyncStatusIcon(const RepoItem *item) const;

    mutable QHash<QString, QString> last_icon_map_;
};


#endif // SEAFILE_CLIENT_REPO_ITEM_DELEGATE_H
