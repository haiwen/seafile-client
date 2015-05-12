#include <QTimer>
#include <QDir>

#include "QtAwesome.h"
#include "utils/utils.h"
#include "seafile-applet.h"
#include "rpc/rpc-client.h"
#include "rpc/clone-task.h"
#include "clone-tasks-table-model.h"

namespace {

const int kUpdateTasksInterval = 1000;

enum {
    COLUMN_NAME = 0,
    COLUMN_WORK_TREE,
    COLUMN_STATE,
    MAX_COLUMN,
};

} // namespace

CloneTasksTableModel::CloneTasksTableModel(QObject *parent)
    : QAbstractTableModel(parent)
{
    update_timer_ = new QTimer(this);
    connect(update_timer_, SIGNAL(timeout()), this, SLOT(updateTasks()));
    update_timer_->start(kUpdateTasksInterval);

    updateTasks();
}

void CloneTasksTableModel::updateTasks()
{
    std::vector<CloneTask> tasks;
    int ret = seafApplet->rpcClient()->getCloneTasks(&tasks);
    if (ret < 0) {
        qDebug("failed to get clone tasks");
        return;
    }

    beginResetModel();
    if (tasks_.size() != tasks.size()) {
        tasks_ = tasks;
        endResetModel();
        return;
    }

    for (int i = 0, n = tasks.size(); i < n; i++) {
        if (tasks_[i] == tasks[i]) {
            continue;
        }

        tasks_[i] = tasks[i];
        QModelIndex start = QModelIndex().child(i, 0);
        QModelIndex stop = QModelIndex().child(i, MAX_COLUMN - 1);
        emit dataChanged(start, stop);
    }
    endResetModel();
}

int CloneTasksTableModel::rowCount(const QModelIndex& parent) const
{
    return tasks_.size();
}

int CloneTasksTableModel::columnCount(const QModelIndex& parent) const
{
    return MAX_COLUMN;
}

QVariant CloneTasksTableModel::data(const QModelIndex & index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    if (role != Qt::DisplayRole) {
        return QVariant();
    }

    const CloneTask &task = tasks_[index.row()];

    int column = index.column();

    if (column == COLUMN_NAME) {
        return task.repo_name;
    } else if (column == COLUMN_WORK_TREE) {
        return QDir::toNativeSeparators(task.worktree);
    } else if (column == COLUMN_STATE) {
        if (task.error_str.length() > 0) {
            return task.error_str;
        } else {
            return task.state_str;
        }
    }

    return QVariant();
}

QVariant CloneTasksTableModel::headerData(int section,
                                          Qt::Orientation orientation,
                                          int role) const
{
    if (orientation == Qt::Vertical) {
        return QVariant();
    }

    if (section == COLUMN_NAME) {
        if (role == Qt::DisplayRole) {
            return tr("Library");
        }
        //else if (role == Qt::DecorationRole) {
        //    return awesome->icon(icon_cloud);
        //}
    } else if (section == COLUMN_WORK_TREE) {
        if (role == Qt::DisplayRole) {
            return tr("Path");
        }
        //else if (role == Qt::DecorationRole) {
        //   return awesome->icon(icon_folder_close_alt);
        //}
    }


    return QVariant();
}

void CloneTasksTableModel::clearSuccessfulTasks()
{
    int n = tasks_.size();
    for (int i = 0; i < n; i++) {
        const CloneTask& task = tasks_[i];
        if (task.isSuccessful()) {
            QString error;
            seafApplet->rpcClient()->removeCloneTask(task.repo_id, &error);
        }
    }
}
