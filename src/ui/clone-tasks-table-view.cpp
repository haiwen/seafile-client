#include <QtGlobal>

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
#include <QtWidgets>
#else
#include <QtGui>
#endif
#include <QHeaderView>
#include <QContextMenuEvent>

#include "QtAwesome.h"
#include "seafile-applet.h"
#include "rpc/rpc-client.h"
#include "rpc/clone-task.h"
#include "clone-tasks-table-model.h"
#include "clone-tasks-table-view.h"


CloneTasksTableHeader::CloneTasksTableHeader(QWidget *parent)
    : QHeaderView(Qt::Horizontal, parent)
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    sectionResizeMode(QHeaderView::Stretch);
#else
    setResizeMode(QHeaderView::Stretch);
#endif
    setHighlightSections(false);
}

CloneTasksTableView::CloneTasksTableView(QWidget *parent)
    : QTableView(parent)
{
    setHorizontalHeader(new CloneTasksTableHeader(this));
    setSelectionBehavior(QAbstractItemView::SelectRows);
    verticalHeader()->hide();

    horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);

    createContextMenu();
}

void CloneTasksTableView::contextMenuEvent(QContextMenuEvent *event)
{
    QPoint pos = event->pos();
    int row = rowAt(pos.y());
    qDebug("row = %d\n", row);
    if (row == -1) {
        return;
    }

    CloneTasksTableModel *model = (CloneTasksTableModel *)this->model();

    CloneTask task = model->taskAt(row);

    prepareContextMenu(task);
    pos = viewport()->mapToGlobal(pos);
    context_menu_->exec(pos);
}

void CloneTasksTableView::prepareContextMenu(const CloneTask& task)
{
    if (task.isCancelable()) {
        cancel_task_action_->setVisible(true);
        cancel_task_action_->setData(task.repo_id);
    } else {
        cancel_task_action_->setVisible(false);
    }

    if (task.isRemovable()) {
        remove_task_action_->setVisible(true);
        remove_task_action_->setData(task.repo_id);
    } else {
        remove_task_action_->setVisible(false);
    }
}

void CloneTasksTableView::createContextMenu()
{
    context_menu_ = new QMenu(this);

    cancel_task_action_ = new QAction(tr("Cancel this task"), this);
    cancel_task_action_->setIcon(awesome->icon(icon_remove));
    cancel_task_action_->setStatusTip(tr("cancel this task"));
    cancel_task_action_->setIconVisibleInMenu(true);
    connect(cancel_task_action_, SIGNAL(triggered()), this, SLOT(cancelTask()));
    context_menu_->addAction(cancel_task_action_);

    remove_task_action_ = new QAction(tr("Remove this task"), this);
    remove_task_action_->setIcon(awesome->icon(icon_remove));
    remove_task_action_->setStatusTip(tr("Remove this task"));
    remove_task_action_->setIconVisibleInMenu(true);
    connect(remove_task_action_, SIGNAL(triggered()), this, SLOT(removeTask()));
    context_menu_->addAction(remove_task_action_);
}

void CloneTasksTableView::cancelTask()
{
    QString repo_id = cancel_task_action_->data().toString();
    QString error;
    if (seafApplet->rpcClient()->cancelCloneTask(repo_id, &error) < 0) {
        QMessageBox::warning(this, getBrand(),
                             tr("Failed to cancel this task:\n\n %1").arg(error),
                             QMessageBox::Ok);
    }
}

void CloneTasksTableView::removeTask()
{
    QString repo_id = remove_task_action_->data().toString();
    QString error;
    if (seafApplet->rpcClient()->removeCloneTask(repo_id, &error) < 0) {
        QMessageBox::warning(this, getBrand(),
                             tr("Failed to remove this task:\n\n %1").arg(error),
                             QMessageBox::Ok);
    }
}
