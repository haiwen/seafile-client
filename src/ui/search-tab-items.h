#ifndef SEAFILE_CLIENT_UI_SEARCH_TAB_ITEMS_H
#define SEAFILE_CLIENT_UI_SEARCH_TAB_ITEMS_H

#include <vector>
#include <QAbstractItemModel>
#include <QListWidgetItem>
#include <QListView>
#include <QStyledItemDelegate>

class QAction;
class QMenu;

class SearchResultListView : public QListView {
    Q_OBJECT
public:
    SearchResultListView(QWidget *parent=0);

protected:
    void contextMenuEvent(QContextMenuEvent *event);

private slots:
    void openParentDir();

private:
    void createActions();
    void updateActions();
    QMenu *prepareContextMenu();

    QAction *open_parent_dir_action_;
};

class SearchResultListModel : public QAbstractListModel {
    Q_OBJECT
public:
    SearchResultListModel();
    ~SearchResultListModel()
    {
        clear();
    }
    int rowCount(const QModelIndex &index) const;
    const QListWidgetItem* item(const QModelIndex& index) const
    {
        if (!index.isValid() || index.row() >= (int)items_.size())
            return NULL;
        return items_[index.row()];
    }
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const
    {
        if (!index.isValid() || index.row() >= (int)items_.size())
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

    void addItem(QListWidgetItem *item);
    const QModelIndex updateSearchResults(const std::vector<QListWidgetItem *> &items, bool is_loading_more, bool has_more);

    const QModelIndex loadMoreIndex() const { return load_more_index_; }

private:
    std::vector<QListWidgetItem*> items_;
    bool has_more_;
    QModelIndex load_more_index_;
};

class SearchResultItemDelegate : public QStyledItemDelegate {
public:
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;
};


#endif // SEAFILE_CLIENT_UI_SEARCH_TAB_ITEMS_H
