#ifndef SEAFILE_CLIENT_FILE_BROWSER_PROGRESS_DIALOG_H
#define SEAFILE_CLIENT_FILE_BROWSER_PROGRESS_DIALOG_H

#include <QProgressDialog>
#include <QObject>
#include <QSharedPointer>

#include "tasks.h"

class QProgressBar;
class QLabel;


class FileBrowserProgressDialog : public QProgressDialog {
    Q_OBJECT
public:
    FileBrowserProgressDialog(FileNetworkTask *task, QWidget *parent=0);

public slots:
    void cancel();

private slots:
    void onProgressUpdate(qint64 processed_bytes, qint64 total_bytes);
    void onTaskFinished(bool success);

private:
    void initTaskInfo();

    QSharedPointer<FileNetworkTask> task_;
    QLabel *description_label_;
    QLabel *more_details_label_;
    QProgressBar *progress_bar_;
};



#endif // SEAFILE_CLIENT_FILE_BROWSER_PROGRESS_DIALOG_H
