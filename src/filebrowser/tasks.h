#ifndef SEAFILE_CLIETN_FILEBROWSER_TAKS_H
#define SEAFILE_CLIETN_FILEBROWSER_TAKS_H

#include <QObject>
#include <QUrl>

#include "account.h"

class QTemporaryFile;
class QFile;
class QNetworkAccessManager;
class QNetworkReply;
class QThread;
class QSslError;

class FileServerTask;
class SeafileApiRequest;
class ApiError;

template<typename T> class QList;

/**
 * Handles file upload/download using seafile web api.
 * The task contains two phases:
 *
 * First, we need to get the upload/download link for seahub.
 * Second, we upload/download file to seafile fileserver with that link using
 * a `FileServerTask`.
 *
 *
 * In second phase, the FileServerTask is moved to a worker thread to execute,
 * since we will do blocking file IO in that task.
 *
 * @abstract
 */
class FileNetworkTask : public QObject {
    Q_OBJECT
public:
    enum TaskType {
        Upload,
        Download,
    };

    FileNetworkTask(const Account& account,
                    const QString& repo_id,
                    const QString& path,
                    const QString& local_path);

    virtual ~FileNetworkTask();

    // accessors
    virtual TaskType type() const = 0;
    QString repoId() const { return repo_id_; };
    QString path() const { return path_; };
    QString localFilePath() const { return local_path_; }
    QString fileName() const;

    enum TaskError {
        NoError = 0,
        ApiRequestError,
        FileIOError,
        TaskCanceled,
    };
    const TaskError& error() const { return error_; }
    const QString& errorString() const { return error_string_; }
    int httpErrorCode() const { return http_error_code_; }

public slots:
    virtual void start();
    virtual void cancel();

signals:
    void progressUpdate(qint64 transferred, qint64 total);
    void finished(bool success);

protected slots:
    virtual void onLinkGet(const QString& link);
    virtual void onGetLinkFailed(const ApiError& error);
    virtual void startFileServerTask(const QString& link);
    virtual void onFileServerTaskFinished(bool success);
    virtual void onFinished(bool success);

protected:
    virtual void createGetLinkRequest() = 0;
    virtual void createFileServerTask(const QString& link) = 0;

    FileServerTask *fileserver_task_;
    SeafileApiRequest *get_link_req_;

    Account account_;
    QString repo_id_;
    QString path_;
    QString local_path_;

    TaskError error_;
    QString error_string_;
    int http_error_code_;
    bool canceled_;

    static QThread *worker_thread_;
};


/**
 * The downloaded file is first written to a tmp location, then moved
 * to its final location.
 */
class FileDownloadTask : public FileNetworkTask {
    Q_OBJECT
public:
    FileDownloadTask(const Account& account,
                     const QString& repo_id,
                     const QString& path,
                     const QString& local_path);

    TaskType type() const { return Download; }
    QString fileId() const { return file_id_; }

protected:
    void createFileServerTask(const QString& link);
    void createGetLinkRequest();

protected slots:
    void onLinkGet(const QString& link);

private:
    QString file_id_;
};

class FileUploadTask : public FileNetworkTask {
    Q_OBJECT
public:
    FileUploadTask(const Account& account,
                   const QString& repo_id,
                   const QString& path,
                   const QString& local_path,
                   bool not_update = true);

    TaskType type() const { return Upload; }

protected:
    void createFileServerTask(const QString& link);
    void createGetLinkRequest();

private:
    bool not_update_;
};

/**
 * Handles raw file download/upload with seafile file server.
 *
 * This is the base class for file upload/download task. The task runs in a
 * seperate thread, which means we can not invoke non-const methods of it from
 * the main thread. To start the task: use QMetaObject::invokeMethod to call
 * the start() member function. task. The same for canceling the task.
 *
 * @abstract
 */
class FileServerTask : public QObject {
    Q_OBJECT
public:
    FileServerTask(const QUrl& url,
                 const QString& local_path);
    virtual ~FileServerTask();

    // accessors
    const FileNetworkTask::TaskError& error() const { return error_; }
    const QString& errorString() const { return error_string_; }
    int httpErrorCode() const { return http_error_code_; }

signals:
    void progressUpdate(qint64 transferred, qint64 total);
    void finished(bool success);

public slots:
    void start();
    void cancel();

protected slots:
    void onSslErrors(const QList<QSslError>& errors);
    void httpRequestFinished();

protected:
    /**
     * Prepare the initialization work, like creating the tmp file, or open
     * the uploaded file for reading.
     */
    virtual void prepare() = 0;

    /**
     * Send the http request.
     * This member function may be called in two places:
     * 1. when task is first started
     * 2. when the request is redirected
     */
    virtual void sendRequest() = 0;
    virtual void onHttpRequestFinished() = 0;
    bool handleHttpRedirect();
    void setError(FileNetworkTask::TaskError error, const QString& error_string);
    void setHttpError(int code);

    static QNetworkAccessManager *network_mgr_;
    QUrl url_;
    QString local_path_;

    QNetworkReply *reply_;
    bool canceled_;
    int redirect_count_;

    FileNetworkTask::TaskError error_;
    QString error_string_;
    int http_error_code_;
};

class GetFileTask : public FileServerTask {
    Q_OBJECT
public:
    GetFileTask(const QUrl& url, const QString& local_path);
    ~GetFileTask();

protected:
    void prepare();
    void sendRequest();
    void onHttpRequestFinished();

private slots:
    void httpReadyRead();

private:
    QTemporaryFile *tmp_file_;
};

class PostFileTask : public FileServerTask {
    Q_OBJECT
public:
    PostFileTask(const QUrl& url,
                 const QString& parent_dir,
                 const QString& local_path,
                 bool not_update);
    ~PostFileTask();

protected:
    void prepare();
    void sendRequest();
    void onHttpRequestFinished();

private:
    QString parent_dir_;
    QFile *file_;
    bool not_update_;
};

#endif // SEAFILE_CLIETN_FILEBROWSER_TAKS_H
