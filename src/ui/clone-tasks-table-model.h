#ifndef SEAFILE_CLIENT_CLONE_TASKS_TABLE_MODEL_H
#define SEAFILE_CLIENT_CLONE_TASKS_TABLE_MODEL_H

#include <QAbstractTableModel>
#include <vector>

#include "rpc/clone-task.h"

class QTimer;

class CloneTasksTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    CloneTasksTableModel(QObject *parent=0);

    int rowCount(const QModelIndex& parent=QModelIndex()) const;
    int columnCount(const QModelIndex& parent=QModelIndex()) const;
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
                                                                                 
    CloneTask taskAt(int i) const { return (i >= (int)tasks_.size()) ? CloneTask() : tasks_[i]; }

public slots:
    void clearSuccessfulTasks();

public slots:
    void updateTasks();

private:

    std::vector<CloneTask> tasks_;
    QTimer *update_timer_;
};


#endif // SEAFILE_CLIENT_CLONE_TASKS_TABLE_MODEL_H
