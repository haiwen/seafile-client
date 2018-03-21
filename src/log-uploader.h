#ifndef SEAFILE_CLIENT_LOG_UPLOADER_H
#define SEAFILE_CLIENT_LOG_UPLOADER_H

#include <QObject>
#include <QRunnable>
#include "account.h"

class QProgressDialog;

class LogDirUploader : public QObject, public QRunnable {
    Q_OBJECT
public:
    LogDirUploader() {};

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
