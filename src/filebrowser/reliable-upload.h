#ifndef SEAFILE_CLIETN_FILEBROWSER_RELIABLE_TAKS_H
#define SEAFILE_CLIETN_FILEBROWSER_RELIABLE_TAKS_H

#include "tasks.h"

class PostFileTask;
class ApiError;
class QBuffer;

// A wrapper of the PostFileTask class to upload a single file. It adds extra
// features like:
// 1. retrying
// 2. chunked uploads: send big files in 1MB chunks, and each chunk is sent with a PostFileTask
// 3. aggregate and show correct progress when retring and chunked uploads
//
// This makes PostFileTask very simple - it only need to focus on uploading a
// single piece of a file, with all uppler level features like retrying and
// chunking implemented here.
class ReliablePostFileTask : public FileServerTask {
    Q_OBJECT
public:
    // Instance created using this constructor would use chunked uploads (if
    // the server supports it).
    ReliablePostFileTask(const Account &account,
                         const QString &repo_id,
                         const QUrl &url,
                         const QString &parent_dir,
                         const QString &local_path,
                         const QString &name,
                         const bool use_upload,
                         const bool accept_user_confirmation);

    // Instance created using this constructor *would not* use chunked uploads.
    // This is used for uploading files in folder uploads.
    ReliablePostFileTask(const QUrl &url,
                         const QString &parent_dir,
                         const QString &local_path,
                         const QString &name,
                         const QString &relative_path);

    ~ReliablePostFileTask();

    virtual const QString &oid() const;
    virtual void continueWithFailedFile(bool retry, const QString& link);

public slots:
    void cancel();

protected:
    bool retryEnabled();
    void prepare();
    void sendRequest();
    void onHttpRequestFinished();
    void checkUploadedBytes();

private slots:
    void onGetFileUploadedBytesSuccess(bool support_chunked_uploading, quint64 uploaded_bytes);
    void onGetFileUploadedBytesFailed(const ApiError& error);
    void onPostFileTaskFinished(bool result);
    void onPostFileTaskProgressUpdate(qint64 done, qint64 total);

private:
    void setContentRangeHeader(QNetworkRequest *request);
    void setupSignals();
    void createPostFileTask();
    void startPostFileTask();
    bool useResumableUpload() const;
    void handlePostFileTaskFailure();

    const Account account_;
    const QString repo_id_;
    const bool part_of_folder_upload_;

    const QString parent_dir_;
    QFile *file_;
    const QString name_;
    const bool use_upload_;
    const QString relative_path_;

    quint64 done_;
    quint64 total_size_;

    //---------------------------------
    // Used for resumable (chunk) uploads
    //---------------------------------
    bool resumable_;
    quint64 current_offset_;
    quint32 current_chunk_size_;
    bool need_idx_progress_;

    // The underlying task that sends the POST file request
    QScopedPointer<PostFileTask, doDeleteLater<PostFileTask> > task_;
    QScopedPointer<GetFileUploadedBytesRequest, doDeleteLater<GetFileUploadedBytesRequest> > file_uploaded_bytes_req_;

    bool accept_user_confirmation_;
};

class PostFileTask : public FileServerTask {
    Q_OBJECT
    friend class ReliablePostFileTask;
public:
    PostFileTask(const QUrl& url,
                 const QString& parent_dir,
                 const QString& local_path,
                 QFile *file,
                 const QString& name,
                 const bool use_upload,
                 quint64 total_size,
                 quint64 start_offset,
                 quint32 chunk_size,
                 const bool need_idx_progress);

    PostFileTask(const QUrl& url,
                 const QString& parent_dir,
                 const QString& local_path,
                 QFile *file,
                 const QString& name,
                 const QString& relative_path,
                 quint64 total_size);
    ~PostFileTask();

protected:
    void prepare();
    void sendRequest();
    void onHttpRequestFinished();

private:
    bool isChunked() const;
    void setContentRangeHeader(QNetworkRequest *request);

    const QString parent_dir_;
    QFile *file_;
    const QString name_;
    const bool use_upload_;
    const QString relative_path_;

    quint64 total_size_;

    quint64 start_offset_;
    qint32 chunk_size_;
    bool need_idx_progress_;
};

#endif // SEAFILE_CLIETN_FILEBROWSER_TAKS_H
