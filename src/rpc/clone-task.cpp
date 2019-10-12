#include <QObject>
#include <QStringList>
#include <glib-object.h>

#include "utils/utils.h"
#include "clone-task.h"
#include "utils/seafile-error.h"

CloneTask CloneTask::fromGObject(GObject *obj)
{
    CloneTask task;

    char *state = NULL;
    int error_code = 0;
    char *repo_id = NULL;
    char *peer_id = NULL;
    char *repo_name = NULL;
    char *worktree = NULL;
    char *tx_id = NULL;

    g_object_get (obj,
                  "state", &state,
                  "error", &error_code,
                  "repo_id", &repo_id,
                  "peer_id", &peer_id,
                  "repo_name", &repo_name,
                  "worktree", &worktree,
                  "tx_id", &tx_id,
                  NULL);

    task.state = QString::fromUtf8(state);
    task.error_code = error_code;
    task.repo_id = QString::fromUtf8(repo_id);
    task.peer_id = QString::fromUtf8(peer_id);
    task.repo_name = QString::fromUtf8(repo_name);
    task.worktree = QString::fromUtf8(worktree);
    task.tx_id = QString::fromUtf8(tx_id);

    task.block_done = 0;
    task.block_total = 0;

    task.checkout_done = 0;
    task.checkout_total = 0;

    g_free (state);
    g_free (repo_id);
    g_free (peer_id);
    g_free (repo_name);
    g_free (worktree);
    g_free (tx_id);

    // task.translateStateInfo();

    return task;
}

QString CloneTask::calcProgress(int done, int total)
{
    if (total == 0) {
        return QString();
    }

    int percentage = done * 100 / total;

    return QString().sprintf(" %d%%", percentage);
}

void CloneTask::translateStateInfo()
{
    if (state == "init") {
        state_str = QObject::tr("initializing...");

    } else if (state == "check server") {
        state_str = QObject::tr("checking server info...");

    } else if (state == "fetch") {
        if (rt_state == "fs") {
            state_str = QObject::tr("Downloading file list...");
            if (fs_objects_total != 0) {
                state_str += calcProgress(fs_objects_done, fs_objects_total);
            }
        } else if (rt_state == "data") {
            state_str = QObject::tr("Downloading files...");
            if (block_total != 0) {
                state_str += calcProgress(block_done, block_total);
            }
        }

    } else if (state == "done") {
        state_str = QObject::tr("Done");

    } else if (state == "canceling") {
        state_str = QObject::tr("Canceling");

    } else if (state == "canceled") {
        state_str = QObject::tr("Canceled");

    } else if (state == "error") {
        error_str = translateSyncErrorCode(error_code);
    } else if (state == "connect") {
        state_str = QObject::tr("connecting server...");

    } else if (state == "index") {
        state_str = QObject::tr("indexing files...");

    } else if (state == "checkout") {
        state_str = QObject::tr("Creating folder...");
        if (checkout_total != 0) {
            state_str += calcProgress(checkout_done, checkout_total);
        }

    } else if (state == "merge") {
        state_str = QObject::tr("Merge file changes...");
    }

    if (state != "error" || error_str == "ok") {
        error_str = "";
    }
}

bool CloneTask::isCancelable() const
{
    QStringList list;
    list << "init" << "connect" << "index" << "fetch" << "error";
    return list.contains(state);
}


bool CloneTask::isRemovable() const
{
    QStringList list;
    list << "done" << "canceled";
    return list.contains(state);
}

bool CloneTask::isDisplayable() const
{
    return state != "canceled" && state != "done";
}
