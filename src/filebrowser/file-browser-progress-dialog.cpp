#include "file-browser-progress-dialog.h"
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QProgressBar>
#include <QPushButton>
#include <QDesktopServices>
#include <QDebug>
#include <QApplication>
#include "utils/utils.h"

FileBrowserProgressDialog::FileBrowserProgressDialog(QWidget *parent)
    : QProgressDialog(parent), mgr_(NULL), task_num_(0), task_done_num_(0)
{
    setWindowModality(Qt::WindowModal);

    QVBoxLayout *layout_ = new QVBoxLayout;
    progress_bar_ = new QProgressBar;
    description_label_ = new QLabel;

    layout_->addWidget(description_label_);
    layout_->addWidget(progress_bar_);

    QHBoxLayout *hlayout_ = new QHBoxLayout;
    more_details_label_ = new QLabel;
    cancel_button_ = new QPushButton(tr("Cancel"));
    QPushButton *hide_button_ = new QPushButton(tr("&Hide"));
    QWidget *spacer = new QWidget;
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    hlayout_->addWidget(more_details_label_);
    hlayout_->addWidget(spacer);
    hlayout_->addWidget(cancel_button_);
    hlayout_->addWidget(hide_button_);
    hlayout_->setSpacing(6);
    hlayout_->setContentsMargins(-1, 0, -1, 6);
    layout_->setContentsMargins(-1, 0, -1, 6);
    layout_->addLayout(hlayout_);

    setLayout(layout_);
    setLabel(description_label_);
    setBar(progress_bar_);
    setCancelButton(cancel_button_);

    connect(hide_button_, SIGNAL(clicked()), this, SLOT(hide()));

    more_details_label_->setText(tr("transferred %1 of %2")
                                   .arg(::readableFileSizeV2(0))
                                   .arg(::readableFileSizeV2(0)));

}

void FileBrowserProgressDialog::setFileNetworkManager(FileNetworkManager *mgr)
{
    if (mgr_)
        disconnect(mgr_, 0, this, 0);

    mgr_ = mgr;

    if (mgr_ == NULL)
        return;

    connect(mgr_, SIGNAL(taskRegistered(const FileNetworkTask *)), this,
            SLOT(onTaskRegistered(const FileNetworkTask *)));

    connect(mgr_, SIGNAL(taskUnregistered(const FileNetworkTask *)), this,
            SLOT(onTaskUnregistered(const FileNetworkTask *)));

    connect(mgr_, SIGNAL(taskProgressed(qint64, qint64)), this,
            SLOT(onTaskProgressed(qint64, qint64)));

    connect(this, SIGNAL(canceled()), this, SLOT(cancel()));
}

void FileBrowserProgressDialog::onTaskRegistered(const FileNetworkTask *task)
{
    if (task == NULL)
        return;

    if (task->type() == SEAFILE_NETWORK_TASK_UPLOAD)
        syncDataAndUI<true>();
    else if (task->type() == SEAFILE_NETWORK_TASK_DOWNLOAD)
        syncDataAndUI<false>();
    else
        return;

    show();
}

void FileBrowserProgressDialog::onTaskUnregistered(const FileNetworkTask *task)
{
    if (!isVisible())
        return;

    if (task->type() == SEAFILE_NETWORK_TASK_UPLOAD)
        syncDataAndUI<true>();
    else if (task->type() == SEAFILE_NETWORK_TASK_DOWNLOAD)
        syncDataAndUI<false>();
    else
        return;

    if (task->status() == SEAFILE_NETWORK_TASK_STATUS_FINISHED &&
        task->type() == SEAFILE_NETWORK_TASK_DOWNLOAD &&
        !openInNativeExtension(task->fileLocation()) &&
        !showInGraphicalShell(task->fileLocation())) {
      qDebug() << Q_FUNC_INFO << task->fileLocation()
               << " is downloaded but unable to open";
      QDesktopServices::openUrl(QUrl::fromLocalFile(
          QFileInfo(task->fileLocation()).dir().absolutePath()));
    }

    // All tasks are finished
    if (task_done_num_ == task_num_) {
        cancel();
        return;
    }
}

void FileBrowserProgressDialog::onTaskProgressed(qint64 bytes, qint64 total_bytes)
{
    if (!isVisible())
        return;

    setValue(bytes);
    setMaximum(total_bytes);

    more_details_label_->setText(tr("transferred %1 of %2")
                                     .arg(::readableFileSizeV2(bytes))
                                     .arg(::readableFileSizeV2(total_bytes)));
}

void FileBrowserProgressDialog::cancel()
{
    mgr_->cancelAll();
    task_num_ = 0;
    task_done_num_ = 0;
    reset();
}

template<bool ISUPLOAD>
void FileBrowserProgressDialog::syncDataAndUI()
{
    if (ISUPLOAD) {
        task_num_ = mgr_->uploadTasks().size();
        task_done_num_ = mgr_->uploadedTaskCount();
        setLabelText(tr("Uploading %1 of %2")
                     .arg(task_done_num_ + 1)
                     .arg(task_num_));
        setWindowTitle(tr("Upload"));
    }
    else {
        task_num_ = mgr_->downloadTasks().size();
        task_done_num_ = mgr_->downloadedTaskCount();
        setLabelText(tr("Downloading %1 of %2")
                     .arg(task_done_num_ + 1)
                     .arg(task_num_));
        setWindowTitle(tr("Download"));
    }
}

