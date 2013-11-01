#ifndef SEAFILE_CLIENT_CLONE_TASKS_DIALOG_H
#define SEAFILE_CLIENT_CLONE_TASKS_DIALOG_H

#include <QDialog>
#include "ui_clone-tasks-dialog.h"

class QTimer;
class QStackedWidget;

class CloneTask;
class CloneTasksTableView;
class CloneTasksTableModel;

class CloneTasksDialog : public QDialog,
                         public Ui::CloneTasksDialog
{
    Q_OBJECT

public:
    CloneTasksDialog(QWidget *parent=0);
    void updateTasks();

private slots:
    void onModelReset();

private:
    void createEmptyView();
    void addTaskItem(const CloneTask& task);

    QStackedWidget *stack_;
    CloneTasksTableView *table_;
    CloneTasksTableModel *model_;
    QWidget *empty_view_;
};


#endif // SEAFILE_CLIENT_CLONE_TASKS_DIALOG_H
