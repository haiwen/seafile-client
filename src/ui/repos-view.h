#ifndef SEAFILE_CLIENT_REPOS_VIEW_H
#define SEAFILE_CLIENT_REPOS_VIEW_H

#include <QWidget>
#include <QHash>
#include "ui_repos-view.h"

class LocalRepo;
class RepoItem;

class ReposView : public QWidget,
                  public Ui::ReposView
{
    Q_OBJECT

public:
    ReposView(QWidget *parent=0);
    void addRepo(const LocalRepo& repo);
    void updateRepos();

private slots:
    void updateRepos(const std::vector<LocalRepo>& repos, bool result);

private:
    Q_DISABLE_COPY(ReposView)

    QWidget *repos_list_;
    QHash<QString, RepoItem*> repos_map_;
};


#endif  // SEAFILE_CLIENT_REPOS_VIEW_H
