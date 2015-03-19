#ifndef SEAFILE_CLIENT_FILE_TABLE_H
#define SEAFILE_CLIENT_FILE_TABLE_H

#include <QTableView>
#include <QStandardItem>
#include <QAbstractTableModel>
#include <QStyledItemDelegate>
#include <QModelIndex>
#include <QScopedPointer>
#include <QSortFilterProxyModel>

#include "api/server-repo.h"
#include "seaf-dirent.h"

class DataManager;

class FileTableViewDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    FileTableViewDelegate(QObject *parent);
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

class FileBrowserDialog;
class FileTableModel;
class FileTableView : public QTableView
{
    Q_OBJECT
public:
    FileTableView(const ServerRepo& repo, QWidget *parent);
    void setModel(QAbstractItemModel *model);

signals:
    void direntClicked(const SeafDirent& dirent);
    void dropFile(const QStringList& paths);
    void direntRename(const SeafDirent& dirent);
    void direntRemove(const SeafDirent& dirent);
    void direntRemove(const QList<const SeafDirent*> &dirents);
    void direntUpdate(const SeafDirent& dirent);
    void direntShare(const SeafDirent& dirent);
    void direntPaste();

    void cancelDownload(const SeafDirent& dirent);
    void syncSubdirectory(const QString& folder_name);

private slots:
    void onAboutToReset();
    void onItemDoubleClicked(const QModelIndex& index);
    void onOpen();
    void onRename();
    void onRemove();
    void onShare();
    void onUpdate();
    void onCopy();
    void onMove();

    void onCancelDownload();
    void onSyncSubdirectory();

private:
    void setupContextMenu();

    // \brief get current selection item
    // it returns non-NULL if only we have one and only one item in seleceted
    // the index it uses internally is mapped to source model
    const SeafDirent *getSelectedItemFromSource();

    // \brief get current selection items
    // it returns the list of all of selected SeafDirents
    // the indexes it uses internally is mapped to source model
    QList<const SeafDirent *> getSelectedItemsFromSource();
    void contextMenuEvent(QContextMenuEvent *event);
    void dropEvent(QDropEvent *event);
    void dragMoveEvent(QDragMoveEvent *event);
    void dragEnterEvent(QDragEnterEvent *event);
    void resizeEvent(QResizeEvent *event);

    Q_DISABLE_COPY(FileTableView)

    // \brief the copy of the exact item where right click event occurs
    // its lifetime is valid during the menu event and exec and invalid outside
    QScopedPointer<const SeafDirent> item_;
    QMenu *context_menu_;
    QMenu *paste_only_menu_;
    QAction *download_action_;
    QAction *rename_action_;
    QAction *remove_action_;
    QAction *share_action_;
    QAction *update_action_;
    QAction *copy_action_;
    QAction *move_action_;
    QAction *paste_action_;
    QAction *cancel_download_action_;
    QAction *sync_subdirectory_action_;
    FileBrowserDialog *parent_;

    // source model
    FileTableModel *source_model_;
    // proxy model
    QSortFilterProxyModel *proxy_model_;
};

class FileTableModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    FileTableModel(QObject *parent=0);

    int rowCount(const QModelIndex& parent=QModelIndex()) const;
    int columnCount(const QModelIndex& parent=QModelIndex()) const;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const;

    void setDirents(const QList<SeafDirent>& dirents);
    const QList<SeafDirent>& dirents() const { return dirents_; }

    const SeafDirent* direntAt(int row) const;

    void insertItem(int pos, const SeafDirent &dirent);
    void appendItem(const SeafDirent &dirent) {
        insertItem(rowCount(), dirent);
    }
    void replaceItem(const QString &name, const SeafDirent &dirent);
    void removeItemNamed(const QString &name);
    void renameItemNamed(const QString &name, const QString &new_name);

    void onResize(const QSize &size);

private slots:
    void updateDownloadInfo();

private:
    Q_DISABLE_COPY(FileTableModel)

    QString getTransferProgress(const SeafDirent& dirent) const;

    QList<SeafDirent> dirents_;

    QHash<QString, QString> progresses_;

    int name_column_width_;

    QTimer *task_progress_timer_;
};

#endif  // SEAFILE_CLIENT_FILE_TABLE_H
