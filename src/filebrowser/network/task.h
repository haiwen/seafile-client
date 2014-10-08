#ifndef SEAFILE_NETWORK_TASK_H
#define SEAFILE_NETWORK_TASK_H

#include <QSharedPointer>
#include "request.h"
class QNetworkReply;
class QSslError;
class QFile;
class QHttpMultiPart;
class QNetworkAccessManager;

typedef enum {
    SEAFILE_NETWORK_TASK_UNKNOWN,
    SEAFILE_NETWORK_TASK_DOWNLOAD,
    SEAFILE_NETWORK_TASK_UPLOAD
} SeafileNetworkTaskType;

typedef enum {
    SEAFILE_NETWORK_TASK_STATUS_UNKNOWN,
    SEAFILE_NETWORK_TASK_STATUS_FRESH,
    SEAFILE_NETWORK_TASK_STATUS_PROCESSING,
    SEAFILE_NETWORK_TASK_STATUS_REDIRECTING,
    SEAFILE_NETWORK_TASK_STATUS_FINISHED,
    SEAFILE_NETWORK_TASK_STATUS_CANCELING,
    SEAFILE_NETWORK_TASK_STATUS_ERROR
} SeafileNetworkTaskStatus;

typedef enum {
    SEAFILE_NETWORK_TASK_UNKNOWN_ERROR,
    SEAFILE_NETWORK_TASK_NO_ERROR,
    SEAFILE_NETWORK_TASK_FILE_ERROR,
    SEAFILE_NETWORK_TASK_NETWORK_ERROR
} SeafileNetworkTaskError;

class SeafileNetworkTask : public QObject {
    Q_OBJECT
public:
    SeafileNetworkTask(const QString &token, const QUrl &url);
    virtual ~SeafileNetworkTask();

    SeafileNetworkTaskStatus status() { return status_; }

    QUrl url() { return url_; }
    void setUrl(const QUrl &url) { url_ = url; }

signals:
    void start();
    void cancel();

protected slots:
    void onStart(); // entry
    void onCancel();
#if !defined(QT_NO_OPENSSL)
    void sslErrors(QNetworkReply*, const QList<QSslError> &errors);
#endif
    void onRedirected(const QUrl &new_url);
    void onAborted(SeafileNetworkTaskError error = SEAFILE_NETWORK_TASK_UNKNOWN_ERROR);

protected:
    void onClose(); // cleanup function, responsible for resource release
    virtual void resume() = 0;
    QNetworkReply *reply_;
    SeafileNetworkRequest *req_;
    QNetworkAccessManager *network_mgr_;

    QString token_;
    QUrl url_;
    SeafileNetworkTaskType type_;
    SeafileNetworkTaskStatus status_;
};

class SeafileDownloadTask : public SeafileNetworkTask {
    Q_OBJECT
    void startRequest();

    QFile *file_; // the file to be download
    QString file_name_; // the file to be download
    QString file_location_; // the absolute path to file
public:
    SeafileDownloadTask(const QString &token,
                        const QUrl &url,
                        const QString &file_name,
                        const QString &file_location);
    ~SeafileDownloadTask();

signals:
    void started();
    void redirected();
    void updateProgress(qint64 processed_bytes, qint64 total_bytes);
    void aborted();
    void finished();

private slots:
    void httpUpdateProgress(qint64 processed_bytes, qint64 total_bytes);
    void httpProcessReady();
    void httpFinished();
    void onRedirected(const QUrl &new_url);
    void onAborted(SeafileNetworkTaskError error = SEAFILE_NETWORK_TASK_UNKNOWN_ERROR);

private:
    void onClose();
    void resume(); // entry for beginning of download
};

class SeafileUploadTask : public SeafileNetworkTask {
    Q_OBJECT
    void startRequest();

    QFile *file_; // the file to be upload
    QString parent_dir_; // the parent dir chose to be upload
    QString file_name_; // the file to be upload
    QString file_location_; // the absolute path to file
    QHttpMultiPart *upload_parts_; // http mutlt part
public:
    SeafileUploadTask(const QString &token,
                        const QUrl &url,
                        const QString &parent_dir,
                        const QString &file_name,
                        const QString &file_location);
    ~SeafileUploadTask();

signals:
    void started();
    void redirected();
    void updateProgress(qint64 processed_bytes, qint64 total_bytes);
    void aborted();
    void finished();

private slots:
    void httpUpdateProgress(qint64 processed_bytes, qint64 total_bytes);
    void httpFinished();
    void onRedirected(const QUrl &new_url);
    void onAborted(SeafileNetworkTaskError error = SEAFILE_NETWORK_TASK_UNKNOWN_ERROR);

private:
    void onClose();
    void resume(); // entry for beginning of upload
};

#endif // SEAFILE_NETWORK_TASK_H
