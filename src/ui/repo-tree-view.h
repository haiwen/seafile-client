#ifndef SEAFILE_CLIENT_REPO_TREE_VIEW_H
#define SEAFILE_CLIENT_REPO_TREE_VIEW_H

#include <vector>
#include <QTreeView>
#include <QSet>

class QAction;
class QContextMenuEvent;
class QEvent;
class QShowEvent;
class QHideEvent;
class QModelIndex;
class QStandardItem;

class RepoItem;
class RepoCategoryItem;
class ServerRepo;
class LocalRepo;

class ApiError;
class CloneTasksDialog;
class FileUploadTask;

class RepoTreeView : public QTreeView {
    Q_OBJECT
public:
    RepoTreeView(QWidget *parent=0);

    std::vector<QAction*> getToolBarActions();

    void expand(const QModelIndex& index, bool remember=true);
    void collapse(const QModelIndex& index, bool remember=true);

    /**
     * Restore the expanded repo categories when:
     * 1. applet startup
     * 1. restore from filtering repos
     */
    void restoreExpandedCategries();
    const QModelIndex getCurrentDropTarget() const
        { return current_drop_target_; }

protected:
    void contextMenuEvent(QContextMenuEvent *event);
    bool viewportEvent(QEvent *event);
    void showEvent(QShowEvent *event);
    void hideEvent(QHideEvent *event);
    void selectionChanged(const QItemSelection &selected,
                          const QItemSelection &deselected);

private slots:
    void downloadRepo();
    void showRepoDetail();
    void openLocalFolder();
    void viewRepoOnWeb();
    void shareRepoToUser();
    void shareRepoToGroup();
    void unshareRepo();
    void onUnshareSuccess();
    void onUnshareFailed(const ApiError& error);
    void openInFileBrowser();
    void onItemClicked(const QModelIndex& index);
    void onItemDoubleClicked(const QModelIndex& index);
    void toggleRepoAutoSync();
    void unsyncRepo();
    void syncRepoImmediately();
    void cancelDownload();
    void loadExpandedCategries();
    void saveExpandedCategries();
    void resyncRepo();
    void setRepoSyncInterval();

    void checkRootPermDone();
    void uploadFileStart(FileUploadTask *task);
    void uploadFileFinished(bool success);
    void copyFileFailed();

private:
    QStandardItem* getRepoItem(const QModelIndex &index) const;

    void createActions();
    QMenu *prepareContextMenu(const RepoItem *item);
    void updateRepoActions();

    void showRepoItemToolTip(const RepoItem *item,
                             const QPoint& pos,
                             const QRect& rect);

    void showRepoCategoryItemToolTip(const RepoCategoryItem *item,
                                     const QPoint& pos,
                                     const QRect& rect);

    void dropEvent(QDropEvent *event);
    void dragMoveEvent(QDragMoveEvent *event);
    void dragEnterEvent(QDragEnterEvent *event);
    void dragLeaveEvent(QDragLeaveEvent *event);
    bool changeGrayBackground(const QPoint& pos,
                              const QRect& rect) const;
    void updateBackground();
    void updateDropTarget(const QModelIndex& index);
    void shareRepo(bool to_group);
    void uploadDroppedFile(const ServerRepo& repo, const QString& path);
    void unsyncRepoImpl(const LocalRepo& repo);

    QAction *download_action_;
    QAction *download_toolbar_action_;
    QAction *show_detail_action_;
    QAction *open_local_folder_action_;
    QAction *open_local_folder_toolbar_action_;
    QAction *unsync_action_;
    QAction *view_on_web_action_;
    QAction *share_repo_to_user_action_;
    QAction *share_repo_to_group_action_;
    QAction *open_in_filebrowser_action_;
    QAction *unshare_action_;
    QAction *toggle_auto_sync_action_;
    QAction *sync_now_action_;
    QAction *cancel_download_action_;
    QAction *resync_action_;
    QAction *set_sync_interval_action_;

    QSet<QString> expanded_categroies_;

    QModelIndex current_drop_target_;
    QModelIndex previous_drop_target_;
};

#endif // SEAFILE_CLIENT_REPO_TREE_VIEW_H
