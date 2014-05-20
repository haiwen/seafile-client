#ifndef SEAFILE_CLIENT_UI_STARRED_FILES_LIST_VIEW_H
#define SEAFILE_CLIENT_UI_STARRED_FILES_LIST_VIEW_H

#include <QListView>

class QAction;
class QContextMenuEvent;
class QMenu;
class QModelIndex;
class QStandardItem;

class StarredFileItem;

class StarredFilesListView : public QListView {
    Q_OBJECT
public:
    StarredFilesListView(QWidget *parent=0);

protected:
    void contextMenuEvent(QContextMenuEvent *event);
//     bool viewportEvent(QEvent *event);
//     void showEvent(QShowEvent *event);
//     void hideEvent(QHideEvent *event);
//     void selectionChanged(const QItemSelection &selected,
//                           const QItemSelection &deselected);

// private slots:
//     void downloadRepo();
//     void showRepoDetail();
//     void openLocalFolder();
//     void viewRepoOnWeb();
//     void onItemClicked(const QModelIndex& index);
//     void onItemDoubleClicked(const QModelIndex& index);
//     void toggleRepoAutoSync();
//     void unsyncRepo();
//     void syncRepoImmediately();
//     void cancelDownload();

private slots:
    void openLocalFile();
    void viewFileOnWeb();

private:
    void createActions();
    QMenu *prepareContextMenu(const StarredFileItem *item);
    void updateActions();
    QStandardItem* getFileItem(const QModelIndex &index) const;

    QAction *open_file_action_;
    QAction *view_file_on_web_action_;
    QAction *open_repo_action_;
    QAction *gen_share_link_action;
};

#endif // SEAFILE_CLIENT_UI_STARRED_FILES_LIST_VIEW_H
