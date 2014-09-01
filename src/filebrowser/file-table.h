#ifndef SEAFILE_CLIENT_FILE_TABLE_H
#define SEAFILE_CLIENT_FILE_TABLE_H

#include <QStandardItemModel>
#include <QStyledItemDelegate>
#include <QModelIndex>

#include "api/server-repo.h"
#include "seaf-dirent.h"

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

    const SeafDirent direntAt(int index) const;

    Qt::ItemFlags flags ( const QModelIndex & index ) const;

private:
    Q_DISABLE_COPY(FileTableModel)

    QList<SeafDirent> dirents_;

    QList<SeafDirent> dirents();
};

enum {
    FILE_COLUMN_NAME = 0,
    FILE_COLUMN_SIZE,
    FILE_COLUMN_MTIME,
    FILE_MAX_COLUMN,
};

#endif  // SEAFILE_CLIENT_FILE_TABLE_H
