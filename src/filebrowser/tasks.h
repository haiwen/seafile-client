#ifndef SEAFILE_CLIETN_FILEBROWSER_TAKS_H
#define SEAFILE_CLIETN_FILEBROWSER_TAKS_H

#include <QObject>
#include <QUrl>
#include <QMutex>

#include "api/server-repo.h"
#include "account.h"
#include "file-browser-requests.h"

class QTemporaryFile;
class QFile;
class QNetworkAccessManager;
class QNetworkReply;
class QThread;
class QSslError;
class QNetworkRequest;

class FileServerTask;
class ApiError;

class ReliablePostFileTask;

template<typename T> class QList;

// deleter
template<typename T>
struct doDeleteLater
{
    static inline void cleanup(T *pointer) {
        if (pointer != NULL)
            pointer->deleteLater();
    }
};

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
class FileNetworkTask: public QObject {
    Q_OBJECT
    friend class ReliablePostFileTask;
public:
    enum TaskType {
        Upload,
        Download
    };

    class Progress {
    public:
        Progress(qint64 transferred, qint64 total);
        QString toString() const;
        qint64 transferred, total;
    };

    FileNetworkTask(const Account& account,
                    const QString& repo_id,
                    const QString& path,
                    const QString& local_path);

    virtual ~FileNetworkTask();

    // accessors
    virtual TaskType type() const = 0;
    const QString& repoId() const { return repo_id_; };
    const Account& account() const { return account_; };
    QString path() const { return path_; };
    QString localFilePath() const { return local_path_; }
    QString failedPath() const { return failed_path_; }
    QString fileName() const;
    QString oid() const { return oid_; }
    QUrl url() const { return url_; }
    Progress progress() const { return progress_; };
    bool canceled() const { return canceled_; }

    enum TaskError {
        NoError = 0,
        ApiRequestError,
        FileIOError,
        TaskCanceled
    };
    const TaskError& error() const { return error_; }
    const QString& errorString() const { return error_string_; }
    int httpErrorCode() const { return http_error_code_; }

public slots:
    virtual void start();
    virtual void cancel();

signals:
    void progressUpdate(qint64 transferred, qint64 total);
    void nameUpdate(QString current_name);
    void finished(bool success);
    void retried(int retry_count);

protected slots:
    void onFileServerTaskProgressUpdate(qint64 transferred, qint64 total);
    void onFileServerTaskNameUpdate(QString current_name);
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

    const Account account_;
    const QString repo_id_;
    QString path_;
    QString local_path_;
    QString failed_path_;
    QString oid_;
    QUrl url_;

    TaskError error_;
    QString error_string_;
    int http_error_code_;
    bool canceled_;

    Progress progress_;

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
                     const QString& local_path,
                     bool is_save_as_task);

    TaskType type() const { return Download; }
    QString fileId() const { return file_id_; }
    bool isSaveAsTask() const { return is_save_as_task_; }

protected:
    void createFileServerTask(const QString& link);
    void createGetLinkRequest();

protected slots:
    void onLinkGet(const QString& link);

private:
    const bool is_save_as_task_;
    QString file_id_;
};

class FileUploadTask : public FileNetworkTask {
    Q_OBJECT
public:
    FileUploadTask(const Account& account,
                   const QString& repo_id,
                   const QString& path,
                   const QString& local_path,
                   const QString& name,
                   const bool use_upload = true,
                   const bool accept_user_confirmation = true);

    // this copy constructor
    // duplicate a task same with the old one, excluding its internal stage
    FileUploadTask(const FileUploadTask& rhs);

    void setAcceptUserConfirmation(bool accept) { accept_user_confirmation_ = accept ;}

    // When a file fails to upload, we ask the user to confirm what he wants to
    // do: retry/skip/abort.
    //
    //  - abort: task->cancel()
    //  - retry: task->continueWithFailedFile(true)
    //  - skip: task->continueWithFailedFile(false)
    void continueWithFailedFile(bool retry);

    // Accessors
    TaskType type() const { return Upload; }
    const QString& name() const { return name_; }
    bool useUpload() const { return use_upload_; }

signals:
    // This signal is meant to be listened by the file progress dialog.
    void oneFileFailed(const QString& filename, bool single_file);

protected slots:
    // Here
    virtual void onOneFileFailed(const QString& filename, bool single_file);
    void onLinkGetAgain(const QString& link);

protected:
    virtual void startFileServerTask(const QString& link);
    void createFileServerTask(const QString& link);
    void createGetLinkRequest();

    const QString name_;

private:
    // the copy assignment, delete it;
    FileUploadTask &operator=(const FileUploadTask& rhs);

    const bool use_upload_;
    bool accept_user_confirmation_;
    bool retry_;
};

class FileUploadMultipleTask : public FileUploadTask {
    Q_OBJECT
public:
    FileUploadMultipleTask(const Account& account,
                           const QString& repo_id,
                           const QString& path,
                           const QString& local_dir_path,
                           const QStringList& names,
                           bool use_upload);

    const QStringList& names() const { return names_; }
    const QStringList& successfulNames();

protected:
    void createFileServerTask(const QString& link);

    const QStringList names_;
};

class FileUploadDirectoryTask : public FileUploadTask {
    Q_OBJECT
public:
    FileUploadDirectoryTask(const Account& account,
                            const QString& repo_id,
                            const QString& path,
                            const QString& local_path,
                            const QString& name);

protected:
    void createFileServerTask(const QString& link);

protected slots:
    // overwrited
    virtual void onFinished(bool success);

private slots:
    void nextEmptyFolder();
    void onCreateDirFailed(const ApiError& error);

private:
    QStringList empty_subfolders_;
    QScopedPointer<CreateDirectoryRequest, QScopedPointerDeleteLater> create_dir_req_;
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

    virtual void continueWithFailedFile(bool retry, const QString& link) {};

    // accessors
    const FileNetworkTask::TaskError& error() const { return error_; }
    const QString& errorString() const { return error_string_; }
    int httpErrorCode() const { return http_error_code_; }
    virtual const QString& oid() const { return oid_; }
    int retryCount() const { return retry_count_; }
    const QString& failedPath() const { return failed_path_; }
    bool canceled() const { return canceled_; }
    QUrl url() const { return url_; }

    static void resetQNAM();

signals:
    void progressUpdate(qint64 transferred, qint64 total);
    void nameUpdate(QString current_name);
    void finished(bool success);
    void retried(int retry_count);

signals:
    void oneFileFailed(const QString& filename, bool single_file);

public slots:
    void start();
    virtual void cancel();

protected slots:
    void onSslErrors(const QList<QSslError>& errors);
    void httpRequestFinished();
    void retry();
    void doAbort();

protected:
    /**
     * Prepare the initialization work, like creating the tmp file, or open the
     * uploaded file for reading. It may be called again when retrying a task.
     */
    virtual void prepare() = 0;

    /**
     * If return true, the request would be retried for a few times when error
     * happened. By default the request is not retried. Subclasses can overwrite
     * this function to enable retry.
     */
    virtual bool retryEnabled();

    /**
     * Send the http request.
     * This member function may be called in two places:
     * 1. when task is first started
     * 2. when the request is redirected
     */
    virtual void sendRequest() = 0;
    virtual void onHttpRequestFinished() = 0;
    bool handleHttpRedirect();
    bool maybeRetry();
    void setError(FileNetworkTask::TaskError error, const QString& error_string);
    void setHttpError(int code);

    // Always use this to access the network access manager, instead of using
    // network_mgr_ directly, because it handles the network status change
    // detection.
    QNetworkAccessManager *getQNAM();

    QUrl url_;
    QString local_path_;
    QString failed_path_;
    QString oid_;

    QNetworkReply *reply_;
    bool canceled_;
    int redirect_count_;
    int retry_count_;

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

class PostFilesTask : public FileServerTask {
    Q_OBJECT
public:
    PostFilesTask(const QUrl& url,
                  const QString& parent_dir,
                  const QString& local_path,
                  const QStringList& names,
                  const bool use_relative);
    ~PostFilesTask();
    int currentNum();

    const QStringList& successfulNames() const { return successful_names_; }

    void continueWithFailedFile(bool retry, const QString& link);

protected:
    void prepare();
    void sendRequest();
    void startNext(const QString& link = QString());

private slots:
    void cancel();
    void onProgressUpdate();
    void onPostTaskProgressUpdate(qint64, qint64);
    void onPostTaskFinished(bool success);

private:
    // never used
    void onHttpRequestFinished() {}

    const QString parent_dir_;
    const QString name_;
    QList<qint64> file_sizes_;
    const QStringList names_;
    QStringList successful_names_;

    QString current_name_;

    QScopedPointer<ReliablePostFileTask, doDeleteLater<ReliablePostFileTask> > task_;
    int current_num_;
    // transferred bytes in the current tasks
    qint64 current_bytes_;
    // the total bytes of completely transferred tasks
    qint64 transferred_bytes_;
    // the total bytes of all tasks
    qint64 total_bytes_;
    // deal with the qt4 eventloop's bug
    QTimer *progress_update_timer_;
    const bool use_relative_;
};

#endif // SEAFILE_CLIETN_FILEBROWSER_TAKS_H
