#ifndef SEAFILE_CLIENT_FILE_TABLE_H
#define SEAFILE_CLIENT_FILE_TABLE_H

#include <QTableView>
#include <QStandardItem>
#include <QAbstractTableModel>
#include <QStyledItemDelegate>
#include <QModelIndex>
#include <QScopedPointer>

#include "api/server-repo.h"
#include "seaf-dirent.h"

class DataManager;

class FileTableViewDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    FileTableViewDelegate(QObject *parent);
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

class FileTableView : public QTableView
{
    Q_OBJECT
public:
    FileTableView(const ServerRepo& repo, QWidget *parent);

signals:
    void direntClicked(const SeafDirent& dirent);
    void dropFile(const QString& file_name);
    void direntRename(const SeafDirent& dirent);
    void direntRemove(const SeafDirent& dirent);
    void direntShare(const SeafDirent& dirent);
    void direntUpdate(const SeafDirent& dirent);
    void cancelDownload(const SeafDirent& dirent);

private slots:
    void onItemDoubleClicked(const QModelIndex& index);
    void onOpen();
    void onRename();
    void onRemove();
    void onShare();
    void onUpdate();
    void onCancelDownload();

private:
    void setupContextMenu();
    void contextMenuEvent(QContextMenuEvent *event);
    void dropEvent(QDropEvent *event);
    void dragMoveEvent(QDragMoveEvent *event);
    void dragEnterEvent(QDragEnterEvent *event);
    void resizeEvent(QResizeEvent *event);

    Q_DISABLE_COPY(FileTableView)

    QScopedPointer<const SeafDirent> item_;
    ServerRepo repo_;
    QMenu *context_menu_;
    QAction *download_action_;
    QAction *update_action_;
    QAction *cancel_download_action_;
    QWidget *parent_;
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

    const SeafDirent* direntAt(int index) const;

    void insertItem(const SeafDirent &dirent);
    void appendItem(const SeafDirent &dirent);
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
