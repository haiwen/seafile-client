#ifndef SEAFILE_CLIENT_CLOUD_VIEW_H
#define SEAFILE_CLIENT_CLOUD_VIEW_H

#include <QWidget>
#include "account.h"

class QPoint;
class QMenu;
class QTimer;
class QShowEvent;
class QHideEvent;

class ServerReposListView;
class ServerReposListModel;
class ServerRepo;

class CloudView : public QWidget
{
    Q_OBJECT
public:
    CloudView(QWidget *parent=0);

protected:
    void showEvent(QShowEvent *event);
    void hideEvent(QHideEvent *event);

public slots:
    void showAddAccountDialog();
    void showSwitchAccountDialog();
    void deleteAccount();

private slots:
    void refreshRepos();
    void refreshRepos(const std::vector<ServerRepo>& repos);
    void refreshReposFailed();
    void setCurrentAccount(const Account&account);

private:
    Q_DISABLE_COPY(CloudView)

    bool hasAccount();

    ServerReposListView *repos_list_;
    ServerReposListModel *repos_model_;

    QTimer *refresh_timer_;
    bool in_refresh_;

    // FolderDropArea *drop_area_;
    Account current_account_;
};


#endif  // SEAFILE_CLIENT_CLOUD_VIEW_H
