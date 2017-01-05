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
    char *err_detail = NULL;
    char *repo_id = NULL;
    char *peer_id = NULL;
    char *repo_name = NULL;
    char *worktree = NULL;
    char *tx_id = NULL;

    g_object_get (obj,
                  "state", &state,
                  "error_str", &error_str,
                  "err_detail", &err_detail,
                  "repo_id", &repo_id,
                  "peer_id", &peer_id,
                  "repo_name", &repo_name,
                  "worktree", &worktree,
                  "tx_id", &tx_id,
                  NULL);

    task.state = QString::fromUtf8(state);
    task.error_str = QString::fromUtf8(error_str);
    task.error_detail = QString::fromUtf8(err_detail);
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
    g_free (err_detail);
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
        if (error_str == "index") {
            error_str = QObject::tr("Failed to index local files");

        } else if (error_str == "check server") {
            error_str = QObject::tr("Failed to check server information");
        } else if (error_str == "checkout") {
            error_str = QObject::tr("Failed to create local files");

        } else if (error_str == "merge") {
            error_str = QObject::tr("Failed to merge local file changes");

        } else if (error_str == "password") {
            error_str = QObject::tr("Incorrect password. Please download again");
        } else if (error_str == "internal") {
            error_str = QObject::tr("Internal error");
        } else if (error_str == "Permission denied on server") {
            error_str = QObject::tr("Permission denied on server. Please try resync the library");
        } else if (error_str == "Network error") {
            error_str = QObject::tr("Network error");
        } else if (error_str == "Cannot resolve proxy address") {
            error_str = QObject::tr("Cannot resolve proxy address");
        } else if (error_str == "Cannot resolve server address") {
            error_str = QObject::tr("Cannot resolve server address");
        } else if (error_str == "Cannot connect to server") {
            error_str = QObject::tr("Cannot connect to server");
        } else if (error_str == "Failed to establish secure connection") {
            error_str = QObject::tr("Failed to establish secure connection. Please check server SSL certificate");
        } else if (error_str == "Data transfer was interrupted") {
            error_str = QObject::tr("Data transfer was interrupted. Please check network or firewall");
        } else if (error_str == "Data transfer timed out") {
            error_str = QObject::tr("Data transfer timed out. Please check network or firewall");
        } else if (error_str == "Unhandled http redirect from server") {
            error_str = QObject::tr("Unhandled http redirect from server. Please check server cofiguration");
        } else if (error_str == "Server error") {
            error_str = QObject::tr("Server error");
        } else if (error_str == "Bad request") {
            error_str = QObject::tr("Bad request");
        } else if (error_str == "Internal data corrupt on the client") {
            error_str = QObject::tr("Internal data corrupt on the client. Please try resync the library");
        } else if (error_str == "Not enough memory") {
            error_str = QObject::tr("Not enough memory");
        } else if (error_str == "Failed to write data on the client") {
            error_str = QObject::tr("Failed to write data on the client. Please check disk space or folder permissions");
        } else if (error_str == "Storage quota full") {
            error_str = QObject::tr("Storage quota full");
        } else if (error_str == "Files are locked by other application") {
            error_str = QObject::tr("Files are locked by other application");
        } else if (error_str == "Library deleted on server") {
            error_str = QObject::tr("Library deleted on server");
        } else if (error_str == "Library damaged on server") {
            error_str = QObject::tr("Library damaged on server");
        }
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
