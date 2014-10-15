#include "file-browser-progress-dialog.h"
#include <QMessageBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QProgressBar>
#include <QPushButton>
#include <QDesktopServices>
#include <QDebug>
#include "utils/utils.h"

FileBrowserProgressDialog::FileBrowserProgressDialog(QWidget *parent)
        : QProgressDialog(parent), task_(NULL)
{
    QVBoxLayout *layout_ = new QVBoxLayout;
    progress_bar_ = new QProgressBar;
    description_label_ = new QLabel;

    layout_->addWidget(description_label_);
    layout_->addWidget(progress_bar_);

    QHBoxLayout *hlayout_ = new QHBoxLayout;
    more_details_label_ = new QLabel;
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
}
void FileBrowserProgressDialog::setTask(const FileNetworkTask *task)
{
    task_ = task;

    setLabelText(((task->type() == SEAFILE_NETWORK_TASK_UPLOAD) ?
       tr("Uploading %1 to \"%2\"") : tr("Downloading %1 to \"%2\"")) \
                 .arg(task->fileName()).arg(task->fileLocation()));
    setWindowTitle((task->type() == SEAFILE_NETWORK_TASK_UPLOAD) ?
       tr("Upload") : tr("Download"));

    more_details_label_->setText("");

    setMaximum(0);
    setValue(0);

    connect(task_, SIGNAL(started()), this, SLOT(onStarted()));
    connect(task_, SIGNAL(updateProgress(qint64, qint64)),
            this, SLOT(onUpdateProgress(qint64, qint64)));
    connect(task_, SIGNAL(aborted()), this, SLOT(onAborted()));
    connect(task_, SIGNAL(finished()), this, SLOT(onFinished()));
    connect(this, SIGNAL(canceled()), task_, SLOT(onCancel()));
}

void FileBrowserProgressDialog::onStarted()
{
    more_details_label_->setText("Pending");
}
void FileBrowserProgressDialog::onUpdateProgress(qint64 processed_bytes, qint64 total_bytes)
{
    if (maximum() != total_bytes)
        setMaximum(total_bytes);
    setValue(processed_bytes);

    more_details_label_->setText(tr("%1 of %2")
                            .arg(::readableFileSizeV2(processed_bytes))
                            .arg(::readableFileSizeV2(total_bytes)));
}
void FileBrowserProgressDialog::onAborted()
{
    disconnect(task_, 0, this, 0);
    more_details_label_->setText(tr("Aborted"));
    QMessageBox::warning(static_cast<QWidget*>(parent()),
                         tr("Aborted"),
                         description_label_->text().append('\n').append(tr("Aborted")));

    reset();
}
void FileBrowserProgressDialog::onFinished()
{
    disconnect(task_, 0, this, 0);
    more_details_label_->setText(tr("Finished"));
    progress_bar_->setValue(maximum());
    if (task_->type() == SEAFILE_NETWORK_TASK_UPLOAD)
        QMessageBox::information(static_cast<QWidget*>(parent()),
                          tr("Finished"),
                          description_label_->text().append('\n').append(tr("Finished")));

    if (task_->type() == SEAFILE_NETWORK_TASK_DOWNLOAD &&
        !QDesktopServices::openUrl(QUrl::fromLocalFile(task_->fileLocation())) &&
        !QDesktopServices::openUrl( \
            QUrl::fromLocalFile(QFileInfo(task_->fileLocation()).dir().absolutePath())))
        qDebug() << Q_FUNC_INFO << task_->fileLocation()
          << " is downloaded but unable to open via openUrl";

    reset();
}
