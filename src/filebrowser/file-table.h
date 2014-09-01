#ifndef SEAFILE_CLIENT_FILE_TABLE_H
#define SEAFILE_CLIENT_FILE_TABLE_H

#include <QStyledItemDelegate>
#include <QModelIndex>

#include "api/server-repo.h"
#include "seaf-dirent.h"

class FileTableModel : public QAbstractTableModel
{
    Q_OBJECT
signals:
    void selectionChanged();

public:
    FileTableModel(QObject *parent=0);

    FileTableModel(const QList<SeafDirent>& dirents, QObject *parent=0);

    int rowCount(const QModelIndex& parent=QModelIndex()) const;
    int columnCount(const QModelIndex& parent=QModelIndex()) const;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const;

    void setDirents(const QList<SeafDirent>& dirents);

    const SeafDirent direntAt(int index) const;

    void setMouseOver(const int row);

    const SeafDirent *selectedDirent() const;

    Qt::DropActions supportedDropActions() const;

private slots:
    void onSelectionChanged(const int row);

private:
    Q_DISABLE_COPY(FileTableModel)

    QList<SeafDirent> dirents_;

    SeafDirent *selected_dirent_;

    QList<SeafDirent> dirents();

    int curr_hovered_;
};

enum {
    FILE_COLUMN_ICON = 0,
    FILE_COLUMN_NAME,
    FILE_COLUMN_MTIME,
    FILE_COLUMN_SIZE,
    FILE_COLUMN_KIND,
    FILE_MAX_COLUMN,
};

#endif  // SEAFILE_CLIENT_FILE_TABLE_H
