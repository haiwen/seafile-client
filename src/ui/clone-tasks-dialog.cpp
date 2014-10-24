#include <QtGlobal>

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
#include <QtWidgets>
#else
#include <QtGui>
#endif
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

//const int kUpdateTasksInterval = 1000;

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
    setWindowIcon(QIcon(":/images/seafile.png"));

    setMinimumSize(QSize(600, 300));

    createEmptyView();

    table_ = new CloneTasksTableView;
    model_ = new CloneTasksTableModel(this);
    table_->setModel(model_);

    stack_ = new QStackedWidget;
    stack_->insertWidget(INDEX_EMPTY_VIEW, empty_view_);
    stack_->insertWidget(INDEX_TASKS_VIEW, table_);

    QVBoxLayout *vlayout = (QVBoxLayout *)layout();
    vlayout->insertWidget(0, stack_);

    mClearBtn->setToolTip(tr("remove all successful tasks"));
    connect(mClearBtn, SIGNAL(clicked()), model_, SLOT(clearSuccessfulTasks()));

    onModelReset();
    connect(model_, SIGNAL(modelReset()), this, SLOT(onModelReset()));
}

void CloneTasksDialog::updateTasks()
{
    model_->updateTasks();
}

void CloneTasksDialog::onModelReset()
{
    if (model_->rowCount() == 0) {
        stack_->setCurrentIndex(INDEX_EMPTY_VIEW);
    } else {
        stack_->setCurrentIndex(INDEX_TASKS_VIEW);
        //table_->justifyColumnWidth();
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
