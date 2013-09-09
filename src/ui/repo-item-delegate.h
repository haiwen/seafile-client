#ifndef SEAFILE_CLIENT_REPO_ITEM_DELEGATE_H
#define SEAFILE_CLIENT_REPO_ITEM_DELEGATE_H

#include <QStyledItemDelegate>

class RepoItem;
class QModelIndex;
class ServerRepo;

class RepoItemDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit RepoItemDelegate(QObject *parent=0);

    void paint(QPainter *painter,
               const QStyleOptionViewItem& option,
               const QModelIndex& index) const;

    QSize sizeHint(const QStyleOptionViewItem& option,
                   const QModelIndex& index) const;

private:
    void matchCurrentItem(const QStyleOptionViewItem& option,
                          const ServerRepo& repo) const;
    RepoItem* getRepoItem(const QModelIndex &index) const;
    void paintRepoItem(QPainter *painter,
                       const QStyleOptionViewItem& opt,
                       const ServerRepo& repo) const;
};


#endif // SEAFILE_CLIENT_REPO_ITEM_DELEGATE_H
