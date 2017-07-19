#ifndef SEAFILE_CLIENT_CLOUD_VIEW_H
#define SEAFILE_CLIENT_CLOUD_VIEW_H

#include <QWidget>
#include "ui_cloud-view.h"

class QTimer;
class QShowEvent;
class QHideEvent;
class QToolButton;
class QToolBar;
class QSizeGrip;
class QTabWidget;
class QUrl;

class SeafileTabWidget;
class ReposTab;
class StarredFilesTab;
class ActivitiesTab;
class SearchTab;
class CloneTasksDialog;
class AccountView;
class Account;

class CloudView : public QWidget,
                  public Ui::CloudView
{
    Q_OBJECT
public:
    CloudView(QWidget *parent=0);

    CloneTasksDialog* cloneTasksDialog();

protected:
    void showEvent(QShowEvent *event);
    void hideEvent(QHideEvent *event);
    void resizeEvent(QResizeEvent *event);
    bool eventFilter(QObject *obj, QEvent *ev);

public slots:
    void showCloneTasksDialog();

private slots:
    void refreshStatusBar();
    void showServerStatusDialog();
    void onRefreshClicked();
    void onMinimizeBtnClicked();
    void onCloseBtnClicked();
    void chooseFolderToSync();
    void onAccountChanged();
    void onTabChanged(int index);
    void refreshServerStatus();
    void onServerLogoFetched(const QUrl& url);
    void onAccountInfoUpdated(const Account& account);

private:
    Q_DISABLE_COPY(CloudView)

    bool hasAccount();

    void setupHeader();
    void setupLogoAndBrand();
    void createAccountView();
    void createTabs();
    void setupDropArea();
    void setupFooter();
    void showProperTabs();

    void refreshTasksInfo();
    void refreshTransferRate();
    void showCreateRepoDialog(const QString& path);
    void updateStorageUsage(const Account& account);

    QTimer *refresh_status_bar_timer_;

    AccountView *account_view_;

    SeafileTabWidget *tabs_;
    ReposTab *repos_tab_;
    StarredFilesTab *starred_files_tab_;
    ActivitiesTab *activities_tab_;
    SearchTab *search_tab_;

    QSizeGrip *resizer_;

    CloneTasksDialog* clone_task_dialog_;
};


#endif  // SEAFILE_CLIENT_CLOUD_VIEW_H
