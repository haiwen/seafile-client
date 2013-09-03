#ifndef SEAFILE_CLIENT_REPOS_TAB_H
#define SEAFILE_CLIENT_REPOS_TAB_H

#include <QWidget>

class ReposView;
class RepoDetailsView;

class ReposTab : public QWidget
{
    Q_OBJECT

public:
    ReposTab(QWidget *parent=0);

private:
    Q_DISABLE_COPY(ReposTab);

    ReposView *repos_view_;
    RepoDetailsView *repo_detail_view_;
};


#endif  // SEAFILE_CLIENT_REPOS_TAB_H
