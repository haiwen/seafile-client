#ifndef SEAFILE_CLIENT_UI_SEARCH_TAB_ITEMS_H
#define SEAFILE_CLIENT_UI_SEARCH_TAB_ITEMS_H
#include <QAbstractItemModel>
#include <QListWidgetItem>
#include <QListView>
#include <QStyledItemDelegate>
#include <vector>

class SearchResultListView : public QListView {
    Q_OBJECT
public:
};

class SearchResultListModel : public QAbstractListModel {
    Q_OBJECT
public:
    ~SearchResultListModel()
    {
        clear();
    }
    int rowCount(const QModelIndex &index) const
    {
        return items_.size();
    }
    const QListWidgetItem* item(const QModelIndex& index) const
    {
        if (!index.isValid() || index.row() >= (int)items_.size())
            return NULL;
        return items_[index.row()];
    }
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const
    {
        if (index.row() >= (int)items_.size())
            return QVariant();
        return items_[index.row()]->data(role);
    }
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) {
        if (index.row() >= (int)items_.size())
            return false;
        if (items_[index.row()]->data(role) == value)
            return true;
        items_[index.row()]->setData(role, value);
        emit dataChanged(index, index);
        return true;
    }
    void clear()
    {
        for (unsigned i = 0 ; i < items_.size(); ++i) {
            delete items_[i];
        }
        items_.clear();
    }
    void addItem(QListWidgetItem *item)
    {
        items_.push_back(item);
        emit dataChanged(index(items_.size() - 1), index(items_.size() - 1));
    }
    void addItems(const std::vector<QListWidgetItem *> &items)
    {
        unsigned first = items_.size();
        items_.insert(items_.end(), items.begin(), items.end());
        emit dataChanged(index(first), index(items_.size() - 1));
    }

private:
    std::vector<QListWidgetItem*> items_;
};

class SearchResultItemDelegate : public QStyledItemDelegate {
public:
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;
};


#endif // SEAFILE_CLIENT_UI_SEARCH_TAB_ITEMS_H
