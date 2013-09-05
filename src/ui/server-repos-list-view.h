#ifndef SEAFILE_CLIENT_SERVER_REPO_LIST_VIEW_H
#define SEAFILE_CLIENT_SERVER_REPO_LIST_VIEW_H

#include <QListView>

class QAction;
class QContextMenuEvent;

class ServerReposListView : public QListView {
    Q_OBJECT
public:
    explicit ServerReposListView(QWidget *parent=0);

protected:
    virtual void contextMenuEvent(QContextMenuEvent *event);

private slots:
    void downloadRepo();
    void syncRepo();

private:
    void createActions();
    void createContextMenu();

    QAction *sync_action_;
    QAction *download_action_;
    QMenu *context_menu_;
};


#endif // SEAFILE_CLIENT_SERVER_REPO_LIST_VIEW_H
