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
    SEAFILE_NETWORK_TASK_UPLOAD,
} SeafileNetworkTaskType;

typedef enum {
    SEAFILE_NETWORK_TASK_STATUS_UNKNOWN,
    SEAFILE_NETWORK_TASK_STATUS_FRESH,
    SEAFILE_NETWORK_TASK_STATUS_PREFETCHING,
    SEAFILE_NETWORK_TASK_STATUS_PREFETCHED,
    SEAFILE_NETWORK_TASK_STATUS_PROCESSING,
    SEAFILE_NETWORK_TASK_STATUS_REDIRECTING,
    SEAFILE_NETWORK_TASK_STATUS_FINISHED,
    SEAFILE_NETWORK_TASK_STATUS_CANCELING,
    SEAFILE_NETWORK_TASK_STATUS_ERROR,
} SeafileNetworkTaskStatus;

typedef enum {
    SEAFILE_NETWORK_TASK_UNKNOWN_ERROR,
    SEAFILE_NETWORK_TASK_NO_ERROR,
    SEAFILE_NETWORK_TASK_FILE_ERROR,
    SEAFILE_NETWORK_TASK_NETWORK_ERROR,
} SeafileNetworkTaskError;

class SeafileNetworkTask : public QObject {
    Q_OBJECT
    void startPrefetchRequest();
    QByteArray *prefetch_api_url_buf_;
public:
    SeafileNetworkTask(const QString &token, const QUrl &url,
                       bool prefetch_api_required = true);
    virtual ~SeafileNetworkTask();

    SeafileNetworkTaskStatus status() { return status_; }

    QUrl url() { return url_; }
    void setUrl(const QUrl &url) { url_ = url; }

signals:
    void start();
    void cancel();
    void resume();
    void prefetchAborted();
    void prefetchFinished();
    void prefetchOid(const QString &oid);

protected slots:
    void onStart();
    void onCancel();
#if !defined(QT_NO_OPENSSL)
    void sslErrors(QNetworkReply*, const QList<QSslError> &errors);
#endif
    void onRedirected(const QUrl &new_url);
    void onAborted(SeafileNetworkTaskError error = SEAFILE_NETWORK_TASK_UNKNOWN_ERROR);
    void onClose();

private slots:
    void onPrefetchProcessReady();
    void onPrefetchFinished();

protected:
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
                        const QString &file_location,
                        bool prefetch_api_required = true);
    ~SeafileDownloadTask();

    QString file_name() { return file_name_; }
    void setFilename(const QString &file_name) { file_name_ = file_name; }
    QString fileLocation() { return file_location_; }
    void setFileLocation(const QString &file_location) {
        file_location_ = file_location;
    }

signals:
    void started();
    void redirected();
    void updateProgress(qint64 processed_bytes, qint64 total_bytes);
    void fileLocationChanged(const QString &);
    void aborted();
    void finished();

private slots:
    void httpUpdateProgress(qint64 processed_bytes, qint64 total_bytes);
    void httpProcessReady();
    void httpFinished();
    void onStartDownload(); // entry for beginning of download
    void onRedirected(const QUrl &new_url);
    void onAborted(SeafileNetworkTaskError error = SEAFILE_NETWORK_TASK_UNKNOWN_ERROR);
    void onClose();
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
                        const QString &file_location,
                        bool prefetch_api_required = true);
    ~SeafileUploadTask();

    QString file_name() { return file_name_; }
    void setFilename(const QString &file_name) { file_name_ = file_name; }
    QString fileLocation() { return file_location_; }
    void setFileLocation(const QString &file_location) {
        file_location_ = file_location;
    }

signals:
    void started();
    void redirected();
    void updateProgress(qint64 processed_bytes, qint64 total_bytes);
    void aborted();
    void finished();

private slots:
    void httpUpdateProgress(qint64 processed_bytes, qint64 total_bytes);
    void httpFinished();
    void onStartUpload(); //entry
    void onRedirected(const QUrl &new_url);
    void onAborted(SeafileNetworkTaskError error = SEAFILE_NETWORK_TASK_UNKNOWN_ERROR);
    void onClose();
};

#endif // SEAFILE_NETWORK_TASK_H
