#ifndef FILE_BROWSER_UI_SEARCH_TAB_H
#define FILE_BROWSER_UI_SEARCH_TAB_H

#include <vector>
#include <QTableView>
#include <QStandardItem>
#include <QAbstractTableModel>
#include <QStyledItemDelegate>
#include <QSortFilterProxyModel>

#include "api/requests.h"

class QAction;
class QMenu;

class FileBrowserSearchItemDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    FileBrowserSearchItemDelegate(QObject *parent);
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

class FileBrowserDialog;
class FileBrowserSearchModel;

class FileBrowserSearchView : public QTableView {
    Q_OBJECT
public:
    FileBrowserSearchView(QWidget *parent);
    void setModel(QAbstractItemModel *model);

    void resizeEvent(QResizeEvent *event);
    void setupContextMenu();
signals:
    void clearSearchBar();
private slots:
    void onAboutToReset();
    void openParentDir();
    void onItemDoubleClicked(const QModelIndex& index);
private:

    Q_DISABLE_COPY(FileBrowserSearchView)

    void contextMenuEvent(QContextMenuEvent *event);

    FileBrowserDialog *parent_;
    FileBrowserSearchModel *search_model_;
    QSortFilterProxyModel *proxy_model_;

    QScopedPointer<const FileSearchResult> search_item_;
    QMenu *context_menu_;
    QAction *open_parent_dir_action_;
};


class FileBrowserSearchModel : public QAbstractTableModel {
    Q_OBJECT
public:
    FileBrowserSearchModel(QObject *parent=0);

    int rowCount(const QModelIndex &parent=QModelIndex()) const;
    int columnCount(const QModelIndex &index) const;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const;

    void setSearchResult(const std::vector<FileSearchResult>& results);

    void onResize(const QSize &size);

    const FileSearchResult* resultAt(int row) const;

private:
    Q_DISABLE_COPY(FileBrowserSearchModel)

    std::vector<FileSearchResult> results_;

    int name_column_width_;
};

class FileSearchSortFilterProxyModel : public QSortFilterProxyModel {
    Q_OBJECT
public:
    FileSearchSortFilterProxyModel(FileBrowserSearchModel *parent)
        : QSortFilterProxyModel(parent), search_model_(parent) {
        setSortCaseSensitivity(Qt::CaseInsensitive);
    }
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const;

private:
    FileBrowserSearchModel* search_model_;
};


#endif // FILE_BROWSER_UI_SEARCH_TAB_H
