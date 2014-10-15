#ifndef SEAFILE_CLIENT_FILE_BROWSER_PROGRESS_DIALOG_H
#define SEAFILE_CLIENT_FILE_BROWSER_PROGRESS_DIALOG_H
#include <QProgressDialog>
#include <QObject>
#include "file-network-mgr.h"
class QProgressBar;
class QLabel;

class FileBrowserProgressDialog : public QProgressDialog {
    Q_OBJECT;
    const FileNetworkTask *task_;
    QLabel *description_label_;
    QLabel *more_details_label_;
    QProgressBar *progress_bar_;
public slots:
    void onStarted();
    void onUpdateProgress(qint64 processed_bytes, qint64 total_bytes);
    void onAborted();
    void onFinished();
public:
    FileBrowserProgressDialog(QWidget *parent = 0);
    void setTask(const FileNetworkTask *task);
};



#endif // SEAFILE_CLIENT_FILE_BROWSER_PROGRESS_DIALOG_H
