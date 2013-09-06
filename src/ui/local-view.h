#ifndef SEAFILE_CLIENT_REPOS_VIEW_H
#define SEAFILE_CLIENT_REPOS_VIEW_H

#include <QWidget>
#include <QHash>

class QTimer;
class QShowEvent;
class QHideEvent;

class LocalRepo;
class LocalReposListView;
class LocalReposListModel;

class LocalView : public QWidget
{
    Q_OBJECT

public:
    LocalView(QWidget *parent=0);

protected:
    void showEvent(QShowEvent *event);
    void hideEvent(QHideEvent *event);

private slots:
    void refreshRepos();

private:
    Q_DISABLE_COPY(LocalView)

    LocalReposListView *repos_list_;
    LocalReposListModel *repos_model_;

    QTimer *refresh_timer_;
    bool in_refresh_;
};


#endif  // SEAFILE_CLIENT_REPOS_VIEW_H
