#ifndef SEAFILE_CLIENT_LOCAL_REPO_LIST_VIEW_H
#define SEAFILE_CLIENT_LOCAL_REPO_LIST_VIEW_H

#include <QListView>



class LocalReposListView : public QListView {
    Q_OBJECT
public:
    explicit LocalReposListView(QWidget *parent=0);

};


#endif // SEAFILE_CLIENT_LOCAL_REPO_LIST_VIEW_H
