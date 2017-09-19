#include <errno.h>
#include <stdio.h>
#include <sqlite3.h>

#include <QDateTime>
#include <QTimer>
#include <QDir>
#include <QDirIterator>

#include "utils/file-utils.h"
#include "utils/utils.h"
#include "configurator.h"
#include "account-mgr.h"
#include "seafile-applet.h"
#include "file-browser-requests.h"
#include "tasks.h"
#include "auto-update-mgr.h"
#include "data-cache.h"
#include "data-mgr.h"

#include "transfer-mgr.h"

namespace {

const int kRefreshRateInterval = 1000;
const char *kPendingStatus = "pending";
const char *kStartedStatus = "started";
const char *kErrorStatus = "error";

bool isTaskForGivenParentDir(const QSharedPointer<FileNetworkTask> &task,
                             const QString& repo_id,
                             const QString& parent_dir)
{
    if (task) {
        if (task->type() == FileNetworkTask::Download) {
            return task->repoId() == repo_id &&
                ::getParentPath(task->path()) == parent_dir;
        } else {
            return task->repoId() == repo_id &&
                task->path() == parent_dir;
        }
    } else {
        return false;
    }
}

bool matchTransferTask(const QSharedPointer<FileNetworkTask> &task,
                       const QString& repo_id,
                       const QString& path,
                       const QString& local_path = QString())
{
    if (!task) {
        return false;
    }

    bool ret = false;
    if (task->type() == FileNetworkTask::Download) {
        ret = task->repoId() == repo_id &&
              task->path() == path;
    } else {
        const QSharedPointer<FileUploadTask> &upload_task =
            qSharedPointerCast<FileUploadTask>(task);
        ret = upload_task->repoId() == repo_id &&
              ::pathJoin(upload_task->path(), upload_task->name()) == path;
        if (!local_path.isEmpty()) {
            ret = ret && (upload_task->localFilePath() == local_path);
        }
    }
    return ret;
}

// bool getFileTaskByIdCB(sqlite3_stmt *stmt, void *data)
// {
//     FileTaskRecord *record = static_cast<FileTaskRecord*>(data);
//     record->id           = sqlite3_column_int (stmt, 0);
//     record->account_sign = (const char *)sqlite3_column_text(stmt, 1);
//     record->type         = (const char *)sqlite3_column_text(stmt, 2);
//     record->repo_id      = (const char *)sqlite3_column_text(stmt, 3);
//     record->path         = (const char *)sqlite3_column_text(stmt, 4);
//     record->local_path   = (const char *)sqlite3_column_text(stmt, 5);
//     record->status       = (const char *)sqlite3_column_text(stmt, 6);
//     record->fail_reason  = (const char *)sqlite3_column_text(stmt, 7);
//     record->parent_task  = sqlite3_column_int (stmt, 8);
//     return true;
// }

bool getFileTasksByIdCB(sqlite3_stmt *stmt, void *data)
{
    QMap <int, FileTaskRecord> *records = static_cast<QMap <int, FileTaskRecord>*>(data);
    FileTaskRecord task;

    task.id           = sqlite3_column_int (stmt, 0);
    task.account_sign = (const char *)sqlite3_column_text(stmt, 1);
    task.type         = (const char *)sqlite3_column_text(stmt, 2);
    task.repo_id      = (const char *)sqlite3_column_text(stmt, 3);
    task.path         = (const char *)sqlite3_column_text(stmt, 4);
    task.local_path   = (const char *)sqlite3_column_text(stmt, 5);
    task.status       = kPendingStatus;
    task.fail_reason  = (const char *)sqlite3_column_text(stmt, 7);
    task.parent_task  = sqlite3_column_int (stmt, 8);

    if (task.isValid()) {
        records->insert(task.id, task);
    }
    return true;
}

bool getFolderTasksByIdCB(sqlite3_stmt *stmt, void *data)
{
    QMap <int, FolderTaskRecord> *records = static_cast<QMap <int, FolderTaskRecord>*>(data);
    FolderTaskRecord task;

    task.id           = sqlite3_column_int (stmt, 0);
    task.account_sign = (const char *)sqlite3_column_text(stmt, 1);
    task.repo_id      = (const char *)sqlite3_column_text(stmt, 2);
    task.path         = (const char *)sqlite3_column_text(stmt, 3);
    task.local_path   = (const char *)sqlite3_column_text(stmt, 4);
    task.status       = kPendingStatus;

    records->insert(task.id, task);
    return true;
}

} // namespace

class TransferProgress {
public:
    TransferProgress();

    void noActivityTask();
    void refreshRate(const quint64 transferred_bytes);
    uint currentRate();
    void onTaskFinished(const quint64 finished_task_size);
    void onTaskStarted(const quint64 new_task_size);

private:
    void updateTransferredBytes(const quint64 finished_task_size);

    // transferred bytes in the current tasks
    quint64 current_bytes;
    quint64 previous_bytes;
    uint current_rate_;

    // the total bytes of completely transferred tasks
    quint64 transferred_bytes;
    // the total bytes of all tasks
    quint64 total_bytes;
};

TransferProgress::TransferProgress()
{
}

void TransferProgress::refreshRate(const quint64 current_task_transferred_bytes)
{
    current_bytes = current_task_transferred_bytes;
    current_rate_ = current_bytes - previous_bytes;
    previous_bytes = current_bytes;
}

uint TransferProgress::currentRate()
{
    return current_rate_;
}

void TransferProgress::noActivityTask()
{
    current_rate_ = 0;
    current_bytes = 0;
    previous_bytes = 0;
}

void TransferProgress::onTaskFinished(const quint64 finished_task_size)
{
    current_rate_ = finished_task_size - previous_bytes;
    current_bytes = 0;
    previous_bytes = 0;
    updateTransferredBytes(finished_task_size);
}

void TransferProgress::updateTransferredBytes(const quint64 finished_task_size)
{
    transferred_bytes += finished_task_size;
}

void TransferProgress::onTaskStarted(const quint64 new_task_size)
{
    total_bytes += new_task_size;
}


SINGLETON_IMPL(TransferManager)

TransferManager::TransferManager()
{
    db_ = NULL;

    download_progress_ = new TransferProgress;
    upload_progress_ = new TransferProgress;

    refresh_rate_timer_ = new QTimer(this);
    connect(refresh_rate_timer_, SIGNAL(timeout()),
            this, SLOT(refreshRate()));
    refresh_rate_timer_->start(kRefreshRateInterval);


    account_signature_ =
        seafApplet->accountManager()->currentAccount().getSignature();
}

TransferManager::~TransferManager()
{
    if (db_) {
        sqlite3_close(db_);
    }
}

bool TransferManager::isValidTask(const FileTaskRecord *task)
{
    if (task == NULL) {
        return false;
    } else if (task->account_sign.isEmpty() || task->type.isEmpty()   ||
               task->repo_id.isEmpty()      || task->path.isEmpty()   ||
               task->local_path.isEmpty()   || task->status.isEmpty() ||
               task->id == 0) {
        return false;
    } else {
        return true;
    }
}

bool TransferManager::isValidTask(const FolderTaskRecord *task)
{
    if (task == NULL) {
        return false;
    } else if (task->account_sign.isEmpty() || task->repo_id.isEmpty() ||
               task->path.isEmpty()         || task->local_path.isEmpty() ||
               task->status.isEmpty()       || task->id == 0 ||
               task->total_bytes == 0) {
        return false;
    } else {
        return true;
    }
}

int TransferManager::start()
{
    const char *errmsg;
    const char *sql;

    QString db_path = QDir(seafApplet->configurator()->seafileDir()).filePath("applet-transfer.db");
    if (sqlite3_open(toCStr(db_path), &db_)) {
        errmsg = sqlite3_errmsg (db_);
        qCritical("failed to open applet-transfer.db %s: %s",
                  toCStr(db_path), errmsg ? errmsg : "no error given");
        seafApplet->errorAndExit(tr("failed to open applet-transfer.db"));
        return -1;
    }

    sql = "CREATE TABLE IF NOT EXISTS FolderTasks ( "
          "id INTEGER PRIMARY KEY, "
          "account_signature TEXT,  repo_id TEXT, path TEXT, "
          "local_path TEXT, status TEXT )";
    if (sqlite_query_exec (db_, sql) < 0) {
        qCritical("failed to create FolderTasks table\n");
        sqlite3_close(db_);
        db_ = NULL;
        return -1;
    }

    sql = "CREATE TABLE IF NOT EXISTS FileTasks ("
          "id INTEGER PRIMARY KEY, "
          "account_signature TEXT, type TEXT, repo_id TEXT, "
          "path TEXT, local_path TEXT, status TEXT, "
          "fail_reason TEXT, parent_task INTEGER )";
    if (sqlite_query_exec (db_, sql) < 0) {
        qCritical("failed to create FileTasks table\n");
        sqlite3_close(db_);
        db_ = NULL;
        return -1;
    }

    getUploadTasksFromDB();
    const FileTaskRecord *pending_file_task = getPendingFileTask();
    if (isValidTask(pending_file_task)) {
        checkUploadName(pending_file_task);
    }

    return 0;
}

FileDownloadTask* TransferManager::addDownloadTask(const Account& account,
                                                   const QString& repo_id,
                                                   const QString& path,
                                                   const QString& local_path,
                                                   bool is_save_as_task)
{
    FileDownloadTask* existing_task = getDownloadTask(repo_id, path);
    if (existing_task) {
        return existing_task;
    }

    FileDownloadTask* task = new FileDownloadTask(account, repo_id, path, local_path, is_save_as_task);
    QSharedPointer<FileDownloadTask> shared_task = task->sharedFromThis().objectCast<FileDownloadTask>();
    connect(task, SIGNAL(finished(bool)),
            this, SLOT(onDownloadTaskFinished(bool)));
    if (current_download_) {
        pending_downloads_.enqueue(shared_task);
    } else {
        startDownloadTask(shared_task);
    }
    return task;
}

void TransferManager::onDownloadTaskFinished(bool success)
{
    if (success) {
        quint64 finished_task_size = current_download_->progress().total;
        download_progress_->onTaskFinished(finished_task_size);
        refresh_rate_timer_->start(kRefreshRateInterval);
        emit taskFinished();
    }

    current_download_.clear();
    if (!pending_downloads_.empty()) {
        const QSharedPointer<FileDownloadTask> &task = pending_downloads_.dequeue();
        startDownloadTask(task);
    }
}

void TransferManager::startDownloadTask(const QSharedPointer<FileDownloadTask> &task)
{
    current_download_ = task;
    task->start();

    refresh_rate_timer_->start(kRefreshRateInterval);
    quint64 current_task_size = current_download_->progress().total;
    download_progress_->onTaskStarted(current_task_size);
}

FileDownloadTask* TransferManager::getDownloadTask(const QString& repo_id,
                                                   const QString& path)
{
    if (matchTransferTask(current_download_, repo_id, path)) {
        return current_download_.data();
    }
    foreach (const QSharedPointer<FileDownloadTask>& task, pending_downloads_) {
        if (matchTransferTask(task, repo_id, path)) {
            return task.data();
        }
    }
    return NULL;
}

void TransferManager::cancelDownload(const QString& repo_id,
                                     const QString& path)
{
    FileDownloadTask* task = getDownloadTask(repo_id, path);
    if (!task)
        return;
    QSharedPointer<FileDownloadTask> shared_task = task->sharedFromThis().objectCast<FileDownloadTask>();
    if (task == current_download_) {
        task->cancel();
    }
    else {
        pending_downloads_.removeOne(shared_task);
    }
}

void TransferManager::cancelAllDownloadTasks()
{
    if (current_download_) {
        current_download_->cancel();
        current_download_.clear();
    }
    pending_downloads_.clear();
}

void TransferManager::cancelAllTasks()
{
    cancelAllDownloadTasks();
    if (current_upload_) {
        current_upload_->cancel();
        current_upload_.clear();
    }
}

FileUploadTask* TransferManager::isCurrentUploadTask(
    const QString& repo_id, const QString& path, const QString& local_path)
{
    if (matchTransferTask(current_upload_, repo_id, path, local_path)) {
        return current_upload_.data();
    } else {
        return NULL;
    }
}

int TransferManager::getUploadTaskId(const QString& repo_id,
                                     const QString& path,
                                     const QString& local_path,
                                     TaskStatus status,
                                     const QString& table)
{
    QString status_str;
    switch (status) {
        case TASK_PENDING: {
            status_str = kPendingStatus;
            break;
        }
        case TASK_STARTED: {
            status_str = kStartedStatus;
            break;
        }
        case TASK_ERROR: {
            status_str = kErrorStatus;
            break;
        }
        default: {
            Q_ASSERT(false);
        }
    }

    if (table == "FileTasks") {
        QMap <int, FileTaskRecord>::const_iterator ite =
            file_tasks_.constBegin();
        while (ite != file_tasks_.constEnd()) {
            if (ite.value().repo_id == repo_id &&
                ite.value().path == path &&
                ite.value().status == status_str) {
                if (local_path.isEmpty()) {
                    return ite.key();
                } else {
                    if (ite.value().local_path == local_path) {
                        return ite.key();
                    }
                }
            }
            ++ite;
        }
    } else {
        QMap <int, FolderTaskRecord>::const_iterator ite =
            folder_tasks_.constBegin();
        while (ite != folder_tasks_.constEnd()) {
            if (ite.value().repo_id == repo_id &&
                ite.value().path == path &&
                ite.value().status == status_str) {
                if (local_path.isEmpty()) {
                    return ite.key();
                } else {
                    if (ite.value().local_path == local_path) {
                        return ite.key();
                    }
                }
            }
            ++ite;
        }
    }

    return 0;
}

const FileTaskRecord* TransferManager::getFileTaskById(int id)
{
    QMap <int, FileTaskRecord>::const_iterator ite =
        file_tasks_.find(id);
    if (ite != file_tasks_.constEnd()) {
        return &ite.value();
    } else {
        return NULL;
    }
}

QList<const FileTaskRecord*> TransferManager::getFileTasksByParentId(int parent_id)
{
    QList<const FileTaskRecord*> ret;

    QMap <int, FileTaskRecord>::const_iterator ite =
        file_tasks_.constBegin();
    while (ite != file_tasks_.constEnd()) {
        if (ite.value().parent_task == parent_id) {
            ret.append(&ite.value());
        }
        ++ite;
    }

    return ret;
}

const FolderTaskRecord* TransferManager::getFolderTaskById(int id)
{
    QMap <int, FolderTaskRecord>::const_iterator ite =
        folder_tasks_.find(id);
    if (ite != folder_tasks_.constEnd()) {
        return &ite.value();
    } else {
        return NULL;
    }
}

void TransferManager::getUploadTasksFromDB()
{
    file_tasks_.clear();
    folder_tasks_.clear();

    if (!db_) {
        return;
    }

    char *zql = sqlite3_mprintf(
        "UPDATE FileTasks SET status = 'pending' "
        "WHERE status = 'started' ");
    sqlite_query_exec(db_, zql);
    sqlite3_free(zql);

    zql = sqlite3_mprintf(
        "UPDATE FolderTasks SET status = 'pending' "
        "WHERE status = 'started' ");
    sqlite_query_exec(db_, zql);
    sqlite3_free(zql);

    zql = sqlite3_mprintf("SELECT * FROM FileTasks WHERE status = 'pending' ");
    sqlite_foreach_selected_row(db_, zql, getFileTasksByIdCB, &file_tasks_);
    sqlite3_free(zql);

    zql = sqlite3_mprintf("SELECT * FROM FolderTasks WHERE status = 'pending' ");
    sqlite_foreach_selected_row(db_, zql, getFolderTasksByIdCB, &folder_tasks_);
    sqlite3_free(zql);

    // update total_bytes of folder tasks
    QMap<int, FolderTaskRecord>::const_iterator ite = folder_tasks_.constBegin();
    while (ite != folder_tasks_.constEnd()) {
        const int folder_task_id = ite.key();
        QMap<int, FileTaskRecord>::const_iterator file_ite = file_tasks_.constBegin();
        while (file_ite != file_tasks_.constEnd()) {
            if (file_ite.value().parent_task == folder_task_id) {
                folder_tasks_[folder_task_id].total_bytes +=
                    QFileInfo(file_ite.value().local_path).size();
            }
            ++file_ite;
        }
        ++ite;
    }
}

const FileTaskRecord *TransferManager::getPendingFileTask()
{
    QMap <int, FileTaskRecord>::const_iterator ite =
        file_tasks_.constBegin();
    while (ite != file_tasks_.constEnd()) {
        if (ite.value().status == kPendingStatus) {
            return &ite.value();
        }
        ++ite;
    }
    return NULL;
}

void TransferManager::addUploadTask(const QString& repo_id,
                                    const QString& repo_parent_path,
                                    const QString& local_path,
                                    const QString& name,
                                    int folder_task_id)
{
    QString repo_full_path = ::pathJoin(repo_parent_path, name);

    if (isCurrentUploadTask(repo_id, repo_full_path, local_path) ||
        getUploadTaskId(repo_id, repo_full_path, local_path)) {
        return;
    }

    QString task_type;
    if (QFileInfo(local_path).isDir()) {
        task_type = "dir";
    } else {
        task_type = "file";
    }

    if (db_) {
        char *zql = NULL;
        if (!folder_task_id) {
            zql = sqlite3_mprintf(
                "REPLACE INTO FileTasks (account_signature, "
                "type, repo_id, path, local_path, status ) "
                "VALUES (%Q, %Q, %Q, %Q, %Q, 'pending') ",
                toCStr(account_signature_),
                toCStr(task_type),
                toCStr(repo_id),
                toCStr(repo_full_path),
                toCStr(local_path));
        } else {
             zql = sqlite3_mprintf(
                "REPLACE INTO FileTasks (account_signature, "
                "type, repo_id, path, local_path, status, parent_task ) "
                "VALUES (%Q, %Q, %Q, %Q, %Q, 'pending', %Q) ",
                toCStr(account_signature_),
                toCStr(task_type),
                toCStr(repo_id),
                toCStr(repo_full_path),
                toCStr(local_path),
                toCStr(QString::number(folder_task_id)));
        }

        qint64 id = 0;
        sqlite_insert_exec(db_, zql, &id);
        sqlite3_free(zql);

        FileTaskRecord params;
        params.id           = id;
        params.account_sign = account_signature_;
        params.type         = task_type;
        params.repo_id      = repo_id;
        params.path         = repo_full_path;
        params.local_path   = local_path;
        params.status       = kPendingStatus;
        params.parent_task  = folder_task_id;
        file_tasks_.insert(id, params);

        checkUploadName(&params);
    }
}

void TransferManager::addUploadTasks(const QString& repo_id,
                                     const QString& repo_parent_path,
                                     const QString& local_parent_path,
                                     const QStringList& names)
{
    for (const QString& name : names) {
        QString local_full_path = ::pathJoin(local_parent_path, name);
        addUploadTask(repo_id, repo_parent_path, local_full_path, name);
    }
}

void TransferManager::addUploadDirectoryTask(const QString& repo_id,
                                             const QString& repo_parent_path,
                                             const QString& local_path,
                                             const QString& name)
{
    QString repo_path = ::pathJoin(repo_parent_path, name);
    qint64 folder_task_id = 0;

    if (db_) {
        char *zql = sqlite3_mprintf(
            "REPLACE INTO FolderTasks (account_signature, "
            "repo_id, path, local_path, status ) "
            "VALUES (%Q, %Q, %Q, %Q, 'pending') ",
            toCStr(account_signature_),
            toCStr(repo_id),
            toCStr(repo_path),
            toCStr(local_path));

        sqlite_insert_exec(db_, zql, &folder_task_id);
        sqlite3_free(zql);
    }

    FolderTaskRecord task;
    task.id           = folder_task_id;
    task.account_sign = account_signature_;
    task.repo_id      = repo_id;
    task.path         = repo_path;
    task.local_path   = local_path;
    task.status       = kPendingStatus;

    if (local_path == "/")
        qWarning("attempt to upload the root directory, you should avoid it\n");
    QDir dir(local_path);

    QDirIterator iterator(dir.absolutePath(), QDirIterator::Subdirectories);
    // XXX (lins05): Move these operations into a thread
    while (iterator.hasNext()) {
        iterator.next();
        QString file_local_path = iterator.filePath();
        QString relative_path = dir.relativeFilePath(file_local_path);
        QString file_repo_path = ::pathJoin(repo_path, relative_path);
        QString file_repo_parent_path = ::getParentPath(file_repo_path);
        QString base_name = ::getBaseName(file_local_path);
        if (base_name == "." || base_name == "..") {
            continue;
        }
        if (iterator.fileInfo().isFile()) {
            addUploadTask(repo_id, file_repo_parent_path,
                          file_local_path, base_name, folder_task_id);
            task.total_bytes += iterator.fileInfo().size();
        } else if (QDir(file_local_path).entryList().length() == 2) {
            // only contains . and .., so an empty folder
            if (seafApplet->accountManager()->currentAccount().isAtLeastVersion(4, 4, 0)) {
                addUploadTask(repo_id, file_repo_parent_path,
                              file_local_path, base_name, folder_task_id);
            }
        }
    }

    if (dir.entryList().length() == 2) {
        // The folder dragged into cloud file browser is an empty one.
        addUploadTask(repo_id, repo_parent_path,
                      local_path, name, folder_task_id);
    }

    folder_tasks_.insert(folder_task_id, task);
}

void TransferManager::checkUploadName(const FileTaskRecord *params)
{
    if (current_upload_ || upload_checking_) {
        return;
    }

    upload_checking_ = true;
    const Account account = seafApplet->accountManager()->
        getAccountBySignature(params->account_sign);
    if (!account.isValid()) {
        return;
    }

    GetFileDetailRequest* req = new GetFileDetailRequest(
        account, params->repo_id, params->path);
    connect(req, SIGNAL(success(const FileDetailInfo&)),
            this, SLOT(onGetFileDetailSuccess(const FileDetailInfo&)));
    connect(req, SIGNAL(failed(const ApiError&)),
            this, SLOT(onGetFileDetailFailed(const ApiError&)));
    req->send();
}

void TransferManager::startUploadTask(const FileTaskRecord *params)
{
    if (!isValidTask(params)) {
        return;
    }

    bool use_relative_path = false;
    const Account account = seafApplet->accountManager()->
        getAccountBySignature(params->account_sign);
    const QString base_name = ::getBaseName(params->local_path);

    FileUploadTask* task = NULL;
    if (params->parent_task) {
        const FolderTaskRecord *folder_params = getFolderTaskById(params->parent_task);
        if (!isValidTask(folder_params)) {
            return;
        }
        updateStatus(folder_params->id, TASK_STARTED, QString(), "FolderTasks");

        if (QFileInfo(params->local_path).isDir()) {
            bool create_parents = true;
            if (folder_params->path == params->path) {
                // This is the case of an empty top-level folder.
                create_parents = false;
            }

            CreateDirectoryRequest* create_dir_req =
                new CreateDirectoryRequest(account, params->repo_id,
                                           params->path, create_parents);
            connect(create_dir_req, SIGNAL(success()),
                    this, SLOT(onCreateDirSuccess()));
            connect(create_dir_req, SIGNAL(failed(const ApiError&)),
                    this, SLOT(onCreateDirFailed(const ApiError&)));
            updateStatus(params->id, TASK_STARTED);
            create_dir_req->send();
            return;
        } else {
            use_relative_path = true;
            const int relative_path_diff = params->local_path.size() -
                                           folder_params->local_path.size();
            const QString relative_path = ::pathJoin(
                QFileInfo(folder_params->local_path).fileName(),
                ::getParentPath(params->local_path.right(relative_path_diff)));
            task = new FileUploadTask(
                account,
                params->repo_id,
                ::getParentPath(folder_params->path),
                params->local_path,
                base_name,
                use_upload_,
                use_relative_path,
                relative_path);
        }
    } else {
        task = new FileUploadTask(
            account,
            params->repo_id,
            ::getParentPath(params->path),
            params->local_path,
            base_name,
            use_upload_);
    }
    task->setId(params->id);
    QSharedPointer<FileUploadTask> shared_task =
        task->sharedFromThis().objectCast<FileUploadTask>();
    connect(task, SIGNAL(finished(bool)),
            this, SLOT(onUploadTaskFinished(bool)));

    if (!current_upload_.isNull()) {
        current_upload_.clear();
    }
    current_upload_ = shared_task;
    updateStatus(current_upload_->id(), TASK_STARTED);
    task->start();
    // emit UploadTaskStarted(current_upload_->id());

    refresh_rate_timer_->start(kRefreshRateInterval);
    quint64 current_task_size = task->progress().total;
    upload_progress_->onTaskStarted(current_task_size);
}

void TransferManager::onCreateDirSuccess()
{
    CreateDirectoryRequest *req = qobject_cast<CreateDirectoryRequest*>(sender());
    if (req != NULL) {
        const QString repo_id = req->repoId();
        const QString path = req->path();
        const int id = getUploadTaskId(repo_id, path, QString(), TASK_STARTED);
        deleteFromDB(id);
    }

    const FileTaskRecord *pending_file_task = getPendingFileTask();
    if (isValidTask(pending_file_task)) {
        checkUploadName(pending_file_task);
    }
}

void TransferManager::onCreateDirFailed(const ApiError& error)
{
    CreateDirectoryRequest *req = qobject_cast<CreateDirectoryRequest*>(sender());
    if (req != NULL) {
        const QString repo_id = req->repoId();
        const QString path = req->path();
        const int id = getUploadTaskId(repo_id, path, QString(), TASK_STARTED);
        updateStatus(id, TASK_ERROR, error.toString());
        emit uploadTaskFailed(getFileTaskById(id));
    }
}

void TransferManager::onGetFileDetailSuccess(const FileDetailInfo& info)
{
    GetFileDetailRequest *req = qobject_cast<GetFileDetailRequest*>(sender());
    if (req != NULL) {
        QMessageBox::StandardButton ret = seafApplet->yesNoCancelBox(
            tr("File %1 already exists.<br/>"
               "Do you like to overwrite it?<br/>"
               "<small>(Choose No to upload using "
               "an alternative name).</small>").arg(req->path()),
            0,
            QMessageBox::Cancel);

        if (ret == QMessageBox::Cancel) {
            cancelUpload(req->repoId(), req->path());
        } else if (ret == QMessageBox::Yes) {
            use_upload_ = false;
        }
        const int id = getUploadTaskId(req->repoId(), req->path());
        const FileTaskRecord* task = getFileTaskById(id);
        startUploadTask(task);
        req->deleteLater();
    }
    upload_checking_ = false;
}

void TransferManager::onGetFileDetailFailed(const ApiError& error)
{
    GetFileDetailRequest *req = qobject_cast<GetFileDetailRequest*>(sender());
    if (req != NULL) {
        use_upload_ = true;
        const int id = getUploadTaskId(req->repoId(), req->path());
        const FileTaskRecord* task = getFileTaskById(id);
        startUploadTask(task);
        req->deleteLater();
    }
    upload_checking_ = false;
}

void TransferManager::onUploadTaskFinished(bool success)
{
    if (success) {
        const FileTaskRecord* task = getFileTaskById(current_upload_->id());
        if (!isValidTask(task)) {
            return;
        }
        emit uploadTaskSuccess(task);
        deleteFromDB(current_upload_->id());

        QFileInfo file_info(current_upload_->localFilePath());
        if (task->parent_task) {
            folder_tasks_[task->parent_task].finished_files_bytes += file_info.size();
        }
        upload_progress_->onTaskFinished(file_info.size());
        refresh_rate_timer_->start(kRefreshRateInterval);
        emit taskFinished();

        if (!current_upload_.isNull()) {
            current_upload_.clear();
        }

        const FileTaskRecord *pending_file_task = getPendingFileTask();
        if (isValidTask(pending_file_task)) {
            checkUploadName(pending_file_task);
        }
    } else {
        updateStatus(current_upload_->id(), TASK_ERROR,
                     current_upload_->errorString());
        emit uploadTaskFailed(getFileTaskById(current_upload_->id()));

        emit taskFinished();

        if (!current_upload_.isNull()) {
            current_upload_.clear();
        }
    }
}

void TransferManager::cancelUpload(const QString& repo_id,
                                   const QString& path)
{
    int task_id = getUploadTaskId(repo_id, path);
    if (task_id) {
        deleteFromDB(task_id);
    } else if (isCurrentUploadTask(repo_id, path)) {
        deleteFromDB(current_upload_->id());
        current_upload_->cancel();

        const FileTaskRecord *pending_file_task = getPendingFileTask();
        if (isValidTask(pending_file_task)) {
            checkUploadName(pending_file_task);
        }

    } else {
        int parent_task_id = getUploadTaskId(repo_id, path, QString(), TASK_PENDING, "FolderTasks");
        if (parent_task_id) {
            deleteFromDB(parent_task_id, "FolderTasks");
        } else {
            parent_task_id = getUploadTaskId(repo_id, path, QString(), TASK_STARTED, "FolderTasks");
            if (parent_task_id) {
                deleteFromDB(parent_task_id, "FolderTasks");

                const FileTaskRecord *pending_file_task = getPendingFileTask();
                if (isValidTask(pending_file_task)) {
                    checkUploadName(pending_file_task);
                }
            }
        }
    }
}

QList<FileNetworkTask*>
TransferManager::getTransferringTasks(const QString& repo_id,
                                      const QString& parent_dir)
{
    QList<FileNetworkTask*> tasks;

    if (isTaskForGivenParentDir(current_download_, repo_id, parent_dir)) {
        tasks.append(current_download_.data());
    }
    if (isTaskForGivenParentDir(current_upload_, repo_id, parent_dir)) {
        tasks.append(current_upload_.data());
    }

    foreach (const QSharedPointer<FileDownloadTask>& task, pending_downloads_) {
        if (isTaskForGivenParentDir(task, repo_id, parent_dir)) {
            tasks.append(task.data());
        }
    }

    return tasks;
}

QList<const FileTaskRecord*> TransferManager::getPendingUploadFiles(
    const QString& repo_id, const QString& parent_dir)
{
    QList<const FileTaskRecord*> ret;

    QMap<int, FileTaskRecord>::const_iterator ite = file_tasks_.constBegin();
    while (ite != file_tasks_.constEnd()) {
        const FileTaskRecord* task = &ite.value();
        if (task->status == kPendingStatus &&
            task->repo_id == repo_id &&
            ::getParentPath(task->path) == parent_dir) {
            ret.push_back(task);
        }
        ++ite;
    }

    return ret;
}

QList<const FolderTaskRecord*> TransferManager::getUploadFolderTasks(
    const QString& repo_id, const QString& parent_dir)
{
    QList<const FolderTaskRecord*> ret;

    QMap<int, FolderTaskRecord>::const_iterator ite = folder_tasks_.constBegin();
    while (ite != folder_tasks_.constEnd()) {
        const FolderTaskRecord* task = &ite.value();
        if (task->repo_id == repo_id &&
            ::getParentPath(task->path) == parent_dir) {
            ret.push_back(task);
        }
        ++ite;
    }

    return ret;
}

bool TransferManager::isTransferring(const QString& repo_id,
                                     const QString& path)
{
    if (getDownloadTask(repo_id, path) ||
        isCurrentUploadTask(repo_id, path) ||
        getUploadTaskId(repo_id, path)) {
        return true;
    } else {
        return false;
    }
}

void TransferManager::cancelTransfer(const QString& repo_id,
                                     const QString& path)
{
    cancelDownload(repo_id, path);
    cancelUpload(repo_id, path);
}

QString TransferManager::getFolderTaskProgress(const QString& repo_id,
                                               const QString& path)
{
    int folder_id = getUploadTaskId(repo_id, path, QString(), TASK_PENDING, "FolderTasks");
    if (folder_id == 0) {
        folder_id = getUploadTaskId(repo_id, path, QString(), TASK_STARTED, "FolderTasks");
    }
    if (folder_id == 0) {
        return QString();
    }

    quint64 total_upload_bytes = folder_tasks_[folder_id].finished_files_bytes;
    if (file_tasks_[current_upload_->id()].parent_task == folder_id) {
        total_upload_bytes += current_upload_->progress().transferred;
    }
    uint progress = total_upload_bytes * 100 / folder_tasks_[folder_id].total_bytes;
    return QString::number(progress) + "%";
}

void TransferManager::getTransferRate(uint *upload_rate,
                                      uint *download_rate)
{
    *upload_rate = upload_progress_->currentRate();
    *download_rate = download_progress_->currentRate();
    return;
}

void TransferManager::refreshRate()
{
    if (current_download_) {
        const quint64 current_task_downloaded_bytes =
            current_download_->progress().transferred;
        download_progress_->refreshRate(current_task_downloaded_bytes);
    } else {
        download_progress_->noActivityTask();
    }

    if (current_upload_) {
        const quint64 current_task_uploaded_bytes =
            current_upload_->progress().transferred;
        upload_progress_->refreshRate(current_task_uploaded_bytes);
    } else {
        upload_progress_->noActivityTask();
    }

    return;
}

void TransferManager::updateStatus(int id,
                                   TaskStatus status,
                                   const QString& error,
                                   const QString& table)
{
    if (!db_) {
        return;
    }

    QString status_str;
    switch (status) {
        case TASK_PENDING: {
            status_str = kPendingStatus;
            break;
        }
        case TASK_STARTED: {
            status_str = kStartedStatus;
            break;
        }
        case TASK_ERROR: {
            status_str = kErrorStatus;
            updateFailReason(id, error);
            const FileTaskRecord* task = getFileTaskById(id);
            if (isValidTask(task) && task->parent_task) {
                char *zql = sqlite3_mprintf(
                    "UPDATE FolderTasks SET status = %Q "
                    "WHERE id = %Q ",
                    toCStr(status_str),
                    toCStr(QString::number(task->parent_task)));
                sqlite_query_exec(db_, zql);
                sqlite3_free(zql);
                zql = sqlite3_mprintf(
                    "UPDATE FileTasks SET status = %Q "
                    "WHERE parent_task = %Q ",
                    toCStr(status_str),
                    toCStr(QString::number(task->parent_task)));
                sqlite_query_exec(db_, zql);
                sqlite3_free(zql);
                file_tasks_[id].status = status_str;
                folder_tasks_[task->parent_task].status = status_str;
            }
            break;
        }
    }

    char *zql = sqlite3_mprintf(
        "UPDATE %Q SET status = %Q "
        "WHERE id = %Q ",
        toCStr(table), toCStr(status_str),
        toCStr(QString::number(id)));
    sqlite_query_exec(db_, zql);
    sqlite3_free(zql);

    if (table == "FileTasks") {
        file_tasks_[id].status = status_str;
    } else {
        folder_tasks_[id].status = status_str;
    }
}

void TransferManager::updateFailReason(int id, const QString& error)
{
    if (!db_) {
        return;
    }

    char *zql = sqlite3_mprintf(
        "UPDATE FileTasks SET fail_reason = %Q "
        "WHERE id = %Q ",
        toCStr(error),
        toCStr(QString::number(id)));
    sqlite_query_exec(db_, zql);
    sqlite3_free(zql);

    file_tasks_[id].fail_reason = error;
}

void TransferManager::deleteFromDB(int id, const QString& table)
{
    if (!db_) {
        return;
    }

    const FileTaskRecord* task = getFileTaskById(id);
    if (!isValidTask(task)) {
       return;
    }
    const int parent_task = task->parent_task;

    char *zql = sqlite3_mprintf(
        "DELETE From %Q WHERE id = %Q ",
        toCStr(table), toCStr(QString::number(id)));
    sqlite_query_exec(db_, zql);
    sqlite3_free(zql);
    file_tasks_.remove(id);

    if (table == "FolderTasks") {
        // upload_folder_size_.remove(id);
        char *zql = sqlite3_mprintf(
            "DELETE From FileTasks WHERE parent_task = %Q ",
            toCStr(QString::number(id)));
        sqlite_query_exec(db_, zql);
        sqlite3_free(zql);

        QMap<int, FileTaskRecord>::const_iterator ite = file_tasks_.constBegin();
        while (ite != file_tasks_.constEnd()) {
            if (ite.value().parent_task == id) {
                file_tasks_.remove(ite.key());
            }
            ++ite;
        }
    } else {
        QList<const FileTaskRecord*> more_sub_tasks = getFileTasksByParentId(parent_task);
        if (more_sub_tasks.isEmpty()) {
            char *zql = sqlite3_mprintf(
                "DELETE From FolderTasks WHERE id = %Q ",
                toCStr(QString::number(parent_task)));
            sqlite_query_exec(db_, zql);
            sqlite3_free(zql);
            folder_tasks_.remove(parent_task);
        }
    }
}
