#ifndef SEAFILE_CLIENT_FILE_BROWSER_PROGRESS_DIALOG_H
#define SEAFILE_CLIENT_FILE_BROWSER_PROGRESS_DIALOG_H
#include <QProgressDialog>
#include <QObject>
#include "file-network-mgr.h"
class QProgressBar;
class FileNetworkManager;
class QLabel;

class FileBrowserProgressDialog : public QProgressDialog {
    Q_OBJECT
public:
    FileBrowserProgressDialog(QWidget *parent = 0);
    void setFileNetworkManager(FileNetworkManager *mgr);

private slots:
    void onTaskRegistered(const FileNetworkTask *task);
    void onTaskUnregistered(const FileNetworkTask *task);
    void onTaskProgressed(qint64 bytes, qint64 total_bytes);

    void cancel();

private:
    //UI
    QLabel *description_label_;
    QLabel *more_details_label_;
    QProgressBar *progress_bar_;
    QPushButton *cancel_button_;

    FileNetworkManager *mgr_;
    unsigned int task_num_;
    unsigned int task_done_num_;
    qint64 task_bytes_;
    qint64 task_done_bytes_;
};



#endif // SEAFILE_CLIENT_FILE_BROWSER_PROGRESS_DIALOG_H
