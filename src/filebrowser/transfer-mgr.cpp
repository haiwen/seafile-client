#include <QDir>
#include <sqlite3.h>
#include <errno.h>
#include <stdio.h>
#include <QDateTime>

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

} // namespace

SINGLETON_IMPL(TransferManager)

TransferManager::TransferManager()
{
    current_task_ = NULL;
}

TransferManager::~TransferManager()
{
}

FileDownloadTask * TransferManager::addDownloadTask(const Account& account,
                                                    const QString& repo_id,
                                                    const QString& path,
                                                    const QString& local_path)
{
    FileDownloadTask *task = new FileDownloadTask(account, repo_id, path, local_path);
    connect(task, SIGNAL(finished(bool)),
            this, SLOT(onDownloadTaskFinished(bool)));
    if (current_task_) {
        download_tasks_.enqueue(task);
    } else {
        task->start();
        current_task_ = task;
    }
    return task;
}

void TransferManager::onFileDownloadFinished(bool success)
{
    if (!download_tasks_.empty()) {
        FileDownloadTask *task = download_tasks_.dequeue();
        task->start();
    }
}
