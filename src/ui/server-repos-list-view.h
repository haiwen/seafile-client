#ifndef SEAFILE_CLIENT_SERVER_REPO_LIST_VIEW_H
#define SEAFILE_CLIENT_SERVER_REPO_LIST_VIEW_H

#include <QListView>



class ServerReposListView : public QListView {
    Q_OBJECT
public:
    explicit ServerReposListView(QWidget *parent=0);

};


#endif // SEAFILE_CLIENT_SERVER_REPO_LIST_VIEW_H
