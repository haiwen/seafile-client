#ifndef SEAFILE_CLIENT_LOG_UPLOADER_H
#define SEAFILE_CLIENT_LOG_UPLOADER_H

#include <QObject>
#include <QRunnable>
#include "account.h"

class ApiError;
class QProgressDialog;
struct UploadLinkInfo;

class LogDirUploader : public QObject, public QRunnable {
    Q_OBJECT
public:
    LogDirUploader() {};

signals:
    void finished(bool success);

public slots:
    void run();

private slots:
    void onUploadLogDirFinished(bool success);
    void onGetUploadLinkSuccess(const QString&);
    void onGetUploadLinkFailed(const ApiError&);
    void onProgressUpdate(qint64, qint64);

private:
    QString compressed_file_name_;
    QProgressDialog *progress_dlg_;
};

#endif // SEAFILE_CLIENT_LOG_UPLOADER_H
