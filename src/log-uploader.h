#ifndef SEAFILE_CLIENT_LOG_UPLOADER_H
#define SEAFILE_CLIENT_LOG_UPLOADER_H

#include <QObject>
#include <QRunnable>
#include <QProgressDialog>
#include "account.h"
#include "filebrowser/reliable-upload.h"

class LogUploadProgressDialog;
class LogDirUploader : public QObject {
    Q_OBJECT
public:
    LogDirUploader();
    ~LogDirUploader();
signals:
    void finished();
public slots:
    virtual void start();
private slots:
    void onCanceled();
    void onCompressFinished(bool success, const QString& compressed_file_name);
    void onGetUploadLinkSuccess(const QString&);
    void onGetUploadLinkFailed(const ApiError&);
    void onUploadLogDirFinished(bool success);
    void onProgressUpdate(qint64 processed_bytes, qint64 total_bytes);
private:
    void onFinished();
    QString compressed_file_name_;
    PostFileTask *task_;
    LogUploadProgressDialog *progress_dlg_;
};

class LogUploadProgressDialog : public QProgressDialog {
    Q_OBJECT
public:
    LogUploadProgressDialog(QWidget *parent=0);
    ~LogUploadProgressDialog();
    void updateData(qint64 processed_bytes, qint64 total_bytes);
};


class LogDirCompressor : public QObject, public QRunnable {
    Q_OBJECT
public:
    LogDirCompressor() {};

signals:
    void compressFinished(bool success,
                          const QString& compressed_file_name = QString());

public slots:
    void run();

private:
    QString compressed_file_name_;
    QProgressDialog *progress_dlg_;
};
#endif // SEAFILE_CLIENT_LOG_UPLOADER_H
