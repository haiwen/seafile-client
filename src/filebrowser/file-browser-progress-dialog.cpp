#include "file-browser-progress-dialog.h"
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QProgressBar>
#include <QPushButton>
#include <QDesktopServices>
#include <QDebug>
#include "utils/utils.h"

FileBrowserProgressDialog::FileBrowserProgressDialog(QWidget *parent)
        : QProgressDialog(parent), task_(NULL), mgr_(NULL)
{
    setWindowModality(Qt::WindowModal);

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

void FileBrowserProgressDialog::setFileNetworkManager(const FileNetworkManager *mgr)
{
    if (mgr_) {
        disconnect(mgr_, 0, this, 0);
        onTaskStarted(NULL);
    }

    mgr_ = mgr;

    if (mgr_ == NULL)
        return;

    connect(mgr_, SIGNAL(taskStarted(const FileNetworkTask*)),
            this, SLOT(onTaskStarted(const FileNetworkTask*)));
}


void FileBrowserProgressDialog::onTaskStarted(const FileNetworkTask *task)
{
    if (task_) {
        disconnect(task_, 0, this, 0);
        hide();
        reset();
    }

    task_ = task;

    if (task_ == NULL)
        return;

    setWindowTitle((task->type() == SEAFILE_NETWORK_TASK_UPLOAD) ?
       tr("Upload") : tr("Download"));
    setLabelText(((task->type() == SEAFILE_NETWORK_TASK_UPLOAD) ?
       tr("Uploading %1") : tr("Downloading %1")) \
                 .arg(task->fileName()));

    more_details_label_->setText("");

    setMaximum(0);
    setValue(0);

    connect(task_, SIGNAL(started()), this, SLOT(onStarted()));
    connect(task_, SIGNAL(updateProgress(qint64, qint64)),
            this, SLOT(onUpdateProgress(qint64, qint64)));
    connect(task_, SIGNAL(aborted()), this, SLOT(onAborted()));
    connect(task_, SIGNAL(finished()), this, SLOT(onFinished()));
    connect(this, SIGNAL(canceled()), task_, SLOT(onCancel()));

    show();
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

    task_ = NULL;
    reset();
}
void FileBrowserProgressDialog::onFinished()
{
    disconnect(task_, 0, this, 0);
    more_details_label_->setText(tr("Finished"));
    progress_bar_->setValue(maximum());

    if (task_->type() == SEAFILE_NETWORK_TASK_DOWNLOAD &&
        !openInNativeExtension(task_->fileLocation()) &&
        !showInGraphicalShell(task_->fileLocation())) {
      qDebug() << Q_FUNC_INFO << task_->fileLocation()
        << " is downloaded but unable to open";
      QDesktopServices::openUrl(QUrl::fromLocalFile(
          QFileInfo(task_->fileLocation()).dir().absolutePath()));
    }

    task_ = NULL;
    reset();
}
