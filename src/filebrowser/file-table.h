#ifndef SEAFILE_CLIENT_FILE_TABLE_H
#define SEAFILE_CLIENT_FILE_TABLE_H

#include <QTableView>
#include <QStandardItem>
#include <QAbstractTableModel>
#include <QStyledItemDelegate>
#include <QModelIndex>

#include "api/server-repo.h"
#include "seaf-dirent.h"

class FileTableView : public QTableView
{
    Q_OBJECT
public:
    FileTableView(const ServerRepo& repo, QWidget *parent=0);

signals:
    void direntClicked(const SeafDirent& dirent);

private slots:
    void onItemDoubleClicked(const QModelIndex& index);

private:
    Q_DISABLE_COPY(FileTableView)

    ServerRepo repo_;
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

    const SeafDirent direntAt(int index) const;

private:
    Q_DISABLE_COPY(FileTableModel)

    QList<SeafDirent> dirents_;
};


#endif  // SEAFILE_CLIENT_FILE_TABLE_H
