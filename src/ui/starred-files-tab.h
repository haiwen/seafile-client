#ifndef SEAFILE_CLIENT_UI_STARRED_FILES_TAB_H
#define SEAFILE_CLIENT_UI_STARRED_FILES_TAB_H

#include <QScopedPointer>
#include "tab-view.h"
#include "api/requests.h"

class QTimer;
class QListWidget;

class ApiError;
class StarredFile;
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
    void refreshStarredFiles(const std::vector<StarredFile>& files);
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
    QWidget *empty_view_;

    QScopedPointer<GetStarredFilesRequest> get_starred_files_req_;
};

#endif // SEAFILE_CLIENT_UI_STARRED_FILES_TAB_H
