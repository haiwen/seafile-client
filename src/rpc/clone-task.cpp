#include <QObject>
#include <QStringList>
#include <glib-object.h>

#include "utils/utils.h"
#include "clone-task.h"

CloneTask CloneTask::fromGObject(GObject *obj)
{
    CloneTask task;

    char *state = NULL;
    char *error_str = NULL;
    char *repo_id = NULL;
    char *peer_id = NULL;
    char *repo_name = NULL;
    char *worktree = NULL;
    char *tx_id = NULL;

    g_object_get (obj,
                  "state", &state,
                  "error_str", &error_str,
                  "repo_id", &repo_id,
                  "peer_id", &peer_id,
                  "repo_name", &repo_name,
                  "worktree", &worktree,
                  "tx_id", &tx_id,
                  NULL);

    task.state = QString::fromUtf8(state);
    task.error_str = QString::fromUtf8(error_str);
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
    g_free (error_str);
    g_free (repo_id);
    g_free (peer_id);
    g_free (repo_name);
    g_free (worktree);
    g_free (tx_id);

    task.translateStateInfo();

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

    } else if (state == "connect") {
        state_str = QObject::tr("connecting server...");

    } else if (state == "index") {
        state_str = QObject::tr("indexing files...");

    } else if (state == "fetch") {
        state_str = QObject::tr("Downloading...");
        if (block_total != 0) {
            state_str += calcProgress(block_done, block_total);
        }

    } else if (state == "checkout") {
        state_str = QObject::tr("Creating folder...");
        if (checkout_total != 0) {
            state_str += calcProgress(checkout_done, checkout_total);
        }

    } else if (state == "merge") {
        state_str = QObject::tr("Merge file changes...");

    } else if (state == "done") {
        state_str = QObject::tr("Done");

    } else if (state == "canceling") {
        state_str = QObject::tr("Canceling");

    } else if (state == "canceled") {
        state_str = QObject::tr("Canceled");

    } else if (state == "error") {
        if (error_str == "index") {
            error_str = QObject::tr("Failed to index local files.");

        } else if (error_str == "checkout") {
            error_str = QObject::tr("Failed to create local files.");

        } else if (error_str == "merge") {
            error_str = QObject::tr("Failed to merge local file changes.");

        } else if (error_str == "password") {
            error_str = QObject::tr("Incorrect password. Please download again.");
        } else if (error_str == "internal") {
            error_str = QObject::tr("Internal error.");
        }
    }

    if (error_str == "ok") {
        error_str = QString();
    }
}

bool CloneTask::isCancelable() const
{
    QStringList list;
    list << "init" << "connect" << "index" << "fetch";
    return list.contains(state);
}


bool CloneTask::isRemovable() const
{
    QStringList list;
    list << "done" << "error" << "canceled";
    return list.contains(state);
}

bool CloneTask::isDisplayable() const
{
    return state != "canceled" && state != "done";
}
