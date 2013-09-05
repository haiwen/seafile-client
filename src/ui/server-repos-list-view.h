#ifndef SEAFILE_CLIENT_SERVER_REPO_LIST_VIEW_H
#define SEAFILE_CLIENT_SERVER_REPO_LIST_VIEW_H

#include <QListView>

class QAction;
class QContextMenuEvent;
class QModelIndex;

class ServerReposListView : public QListView {
    Q_OBJECT
public:
    explicit ServerReposListView(QWidget *parent=0);

protected:
    virtual void contextMenuEvent(QContextMenuEvent *event);

private slots:
    void downloadRepo();
    void syncRepo();
    void showRepoDetail();

private:
    void createActions();
    void createContextMenu();
    void prepareContextMenu(const QModelIndex& index);
    bool hasLocalRepo(const QString& repo_id);

    QMenu *context_menu_;
    QAction *sync_action_;
    QAction *download_action_;
    QAction *show_detail_action_;
};


#endif // SEAFILE_CLIENT_SERVER_REPO_LIST_VIEW_H
