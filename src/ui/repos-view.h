#ifndef SEAFILE_CLIENT_REPOS_VIEW
#define SEAFILE_CLIENT_REPOS_VIEW

#include <QWidget>
#include "ui_repos-view.h"

class LocalRepo;

class ReposView : public QWidget,
                  public Ui::ReposView
{
    Q_OBJECT

public:
    ReposView(QWidget *parent=0);
    void addRepo(const LocalRepo& repo);
    void updateRepos();

private:
    Q_DISABLE_COPY(ReposView)

    QWidget *repos_list_;
};


#endif  // SEAFILE_CLIENT_REPOS_VIEW
