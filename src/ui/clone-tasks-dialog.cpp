#include <QtGui>
#include <QTableView>
#include <QTimer>

#include "utils/utils.h"
#include "seafile-applet.h"
#include "rpc/rpc-client.h"
#include "rpc/clone-task.h"
#include "clone-tasks-table-model.h"
#include "clone-tasks-table-view.h"
#include "clone-tasks-dialog.h"

namespace {

const int kUpdateTasksInterval = 1000;

enum {
    INDEX_EMPTY_VIEW = 0,
    INDEX_TASKS_VIEW
};

} // namespace


CloneTasksDialog::CloneTasksDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUi(this);

    setWindowTitle(tr("Download tasks"));

    setMinimumSize(QSize(500, 300));

    createEmptyView();

    table_ = new CloneTasksTableView;
    model_ = new CloneTasksTableModel(this);
    table_->setModel(model_);

    stack_ = new QStackedWidget;
    stack_->insertWidget(INDEX_EMPTY_VIEW, empty_view_);
    stack_->insertWidget(INDEX_TASKS_VIEW, table_);

    QVBoxLayout *vlayout = (QVBoxLayout *)layout();
    vlayout->insertWidget(0, stack_);

    onModelReset();
    connect(model_, SIGNAL(modelReset()), this, SLOT(onModelReset()));
}

void CloneTasksDialog::onModelReset()
{
    if (model_->rowCount() == 0) {
        stack_->setCurrentIndex(INDEX_EMPTY_VIEW);
    } else {
        stack_->setCurrentIndex(INDEX_TASKS_VIEW);
        table_->resizeColumnsToContents();
    }
}
        

void CloneTasksDialog::createEmptyView()
{
    empty_view_ = new QWidget(this);

    QVBoxLayout *layout = new QVBoxLayout;
    empty_view_->setLayout(layout);

    QLabel *label = new QLabel;
    label->setText(tr("No download tasks right now."));
    label->setAlignment(Qt::AlignCenter);

    layout->addWidget(label);
}
