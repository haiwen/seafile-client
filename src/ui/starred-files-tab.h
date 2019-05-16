#ifndef SEAFILE_CLIENT_UI_STARRED_FILES_TAB_H
#define SEAFILE_CLIENT_UI_STARRED_FILES_TAB_H

#include "tab-view.h"

class QTimer;
class QListWidget;

class GetStarredFilesRequest;
class GetStarredFilesRequestV2;
class ApiError;
class StarredItem;
class StarredFilesListView;
class StarredFilesListModel;

/**
 * The starred files tab
 */
class StarredFilesTab : public TabView {
    Q_OBJECT
public:
    explicit StarredFilesTab(QWidget *parent=0);

public slots:
    void refresh();

protected:
    void startRefresh();
    void stopRefresh();

private slots:
    void refreshStarredFiles(const std::vector<StarredItem>& files);
    void refreshStarredFilesV2(const std::vector<StarredItem>& files);
    void refreshStarredFilesFailed(const ApiError& error);

private:
    void createStarredFilesListView();
    void createLoadingView();
    void createLoadingFailedView();
    void createEmptyView();
    void showLoadingView();

    QTimer *refresh_timer_;
    bool in_refresh_;

    StarredFilesListView *files_list_view_;
    StarredFilesListModel *files_list_model_;

    QWidget *loading_view_;
    QWidget *loading_failed_view_;
    QWidget *logout_view_;
    QWidget *empty_view_;

    GetStarredFilesRequest *get_starred_files_req_;
    GetStarredFilesRequestV2 *get_starred_items_req_;
};

#endif // SEAFILE_CLIENT_UI_STARRED_FILES_TAB_H
