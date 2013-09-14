#ifndef SEAFILE_CLIENT_CLONE_TASKS_TABLE_VIEW_H
#define SEAFILE_CLIENT_CLONE_TASKS_TABLE_VIEW_H

#include <QTableView>
#include <QHeaderView>

class QAction;
class QMenu;
class QContextMenuEvent;

class CloneTask;

class CloneTasksTableView : public QTableView
{
    Q_OBJECT

public:
    CloneTasksTableView(QWidget *parent=0);

    void contextMenuEvent(QContextMenuEvent *event);

private slots:
    void cancelTask();
    void removeTask();

private:
    void createContextMenu();
    void prepareContextMenu(const CloneTask& task);

    QAction *cancel_task_action_;
    QAction *remove_task_action_;
    QMenu *context_menu_;
};

class CloneTasksTableHeader : public QHeaderView {
    Q_OBJECT
    
public:
    CloneTasksTableHeader(QWidget *parent=0);
};

#endif // SEAFILE_CLIENT_CLONE_TASKS_TABLE_VIEW_H
