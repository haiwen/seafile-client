#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QProgressBar>
#include <QPushButton>
#include <QDesktopServices>
#include <QDebug>

#include "utils/utils.h"
#include "progress-dialog.h"

FileBrowserProgressDialog::FileBrowserProgressDialog(FileNetworkTask *task, QWidget *parent)
        : QProgressDialog(parent),
          task_(task)
{
    setWindowModality(Qt::WindowModal);

    QVBoxLayout *layout_ = new QVBoxLayout;
    progress_bar_ = new QProgressBar;
    description_label_ = new QLabel;

    layout_->addWidget(description_label_);
    layout_->addWidget(progress_bar_);

    QHBoxLayout *hlayout_ = new QHBoxLayout;
    more_details_label_ = new QLabel;
    more_details_label_->setText("Pending");
    QPushButton *cancel_button_ = new QPushButton(tr("Cancel"));
    QWidget *spacer = new QWidget;
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    hlayout_->addWidget(more_details_label_);
    hlayout_->addWidget(spacer);
    hlayout_->addWidget(cancel_button_);
    hlayout_->setContentsMargins(-1, 0, -1, 6);
    layout_->setContentsMargins(-1, 0, -1, 6);
    layout_->addLayout(hlayout_);

    setLayout(layout_);
    setLabel(description_label_);
    setBar(progress_bar_);
    setCancelButton(cancel_button_);

    initTaskInfo();
}

void FileBrowserProgressDialog::initTaskInfo()
{
    QString title, label;
    if (task_->type() == FileNetworkTask::Upload) {
        title = tr("Upload");
        label = tr("Uploading %1");
    } else {
        title = tr("Download");
        label = tr("Downloading %1");
    }
    setWindowTitle(title);
    setLabelText(label.arg(task_->fileName()));

    more_details_label_->setText("");

    setMaximum(0);
    setValue(0);

    connect(task_.data(), SIGNAL(progressUpdate(qint64, qint64)),
            this, SLOT(onProgressUpdate(qint64, qint64)));
    connect(task_.data(), SIGNAL(finished(bool)), this, SLOT(onTaskFinished(bool)));
    connect(this, SIGNAL(canceled()), task_.data(), SLOT(cancel()));

    show();
}

void FileBrowserProgressDialog::onProgressUpdate(qint64 processed_bytes, qint64 total_bytes)
{
    if (maximum() != total_bytes)
        setMaximum(total_bytes);
    setValue(processed_bytes);

    more_details_label_->setText(tr("%1 of %2")
                            .arg(::readableFileSizeV2(processed_bytes))
                            .arg(::readableFileSizeV2(total_bytes)));
}

void FileBrowserProgressDialog::onTaskFinished(bool success)
{
    if (success) {
        // printf ("progress dialog: task success\n");
        accept();
    } else {
        // printf ("progress dialog: task failed\n");
        reject();
    }
}

void FileBrowserProgressDialog::cancel()
{
    task_->cancel();
    reject();
}
