#ifndef SEAFILE_CLIENT_CLOUD_VIEW_H
#define SEAFILE_CLIENT_CLOUD_VIEW_H

#include <QWidget>
#include "ui_cloud-view.h"

class QPoint;
class QMenu;
class ServerReposView;

class CloudView : public QWidget,
                  public Ui::CloudView
{
    Q_OBJECT

public:
    CloudView(QWidget *parent=0);

private slots:
    void switchAccount();
    void addAccount();
    void deleteAccount();

private:
    Q_DISABLE_COPY(CloudView)

    void createAccoutOpMenu();
    void prepareAccountOpButton();

    ServerReposView *server_repos_view_;

    QMenu *accout_op_menu_;
    // Account operations
    QAction *add_account_action_;
    QAction *delete_account_action_;
    QAction *switch_account_action_;

    // FolderDropArea *drop_area_;
};


#endif  // SEAFILE_CLIENT_CLOUD_VIEW_H
