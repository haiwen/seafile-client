#ifndef SEAFILE_CLIENT_RPC_CLONE_TASK_H
#define SEAFILE_CLIENT_RPC_CLONE_TASK_H

#include <QString>
#include <QMetaType>

struct _GObject;

class CloneTask {
public:
    QString repo_id;
    QString repo_name;
    QString worktree;
    QString state;
    QString error_str;
    QString peer_id;
    QString tx_id;

    QString state_str;
    int block_done;
    int block_total;

    int checkout_done;
    int checkout_total;

    static CloneTask fromGObject(_GObject *obj);

    void translateStateInfo();

    bool isCancelable() const;
    bool isRemovable() const;
    bool isDisplayable() const;
    bool isSuccessful() const { return state == "done"; }

    bool isValid() const { return !repo_id.isEmpty(); }

    bool operator==(const CloneTask& rhs) const {
        return repo_id == rhs.repo_id
            && repo_name == rhs.repo_name
            && worktree == rhs.worktree
            && state_str == rhs.state_str
            && error_str == rhs.error_str;
    }

    bool operator!=(const CloneTask& rhs) const {
        return !(*this == rhs);
    }

private:
    QString calcProgress(int done, int total);
};

#endif // SEAFILE_CLIENT_RPC_CLONE_TASK_H
