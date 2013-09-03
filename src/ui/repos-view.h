#ifndef SEAFILE_CLIENT_REPOS_VIEW_H
#define SEAFILE_CLIENT_REPOS_VIEW_H

#include <QWidget>
#include <QHash>
#include "ui_repos-view.h"

class QTimer;
class QShowEvent;
class QHideEvent;

class LocalRepo;
class RepoItem;

class ReposView : public QWidget,
                  public Ui::ReposView
{
    Q_OBJECT

public:
    ReposView(QWidget *parent=0);
    void addRepo(const LocalRepo& repo);

protected:
    void showEvent(QShowEvent *event);
    void hideEvent(QHideEvent *event);

private slots:
    void refreshRepos(const std::vector<LocalRepo>& repos, bool result);
    void refreshRepos();

private:
    Q_DISABLE_COPY(ReposView)

    QWidget *repos_list_;
    QHash<QString, RepoItem*> repos_map_;

    QTimer *refresh_timer_;
    bool in_refresh_;
};


#endif  // SEAFILE_CLIENT_REPOS_VIEW_H
