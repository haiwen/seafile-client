#ifndef SEAFILE_CLIENT_UI_SEARCH_TAB_H
#define SEAFILE_CLIENT_UI_SEARCH_TAB_H
#include "tab-view.h"

class QWidget;
class QLabel;
class QListView;
class SearchResultListView;
class SearchResultListModel;
class SearchResultItemDelegate;
class QListWidget;
class QListWidgetItem;
class QLineEdit;
class QTimer;
class QModelIndex;
class FileSearchRequest;
struct FileSearchResult;
class ApiError;
class LoadMoreButton;
class SearchBar;

class SearchTab : public TabView {
    Q_OBJECT
public:
    explicit SearchTab(QWidget *parent = 0);
    ~SearchTab();
    void reset();

public slots:
    void refresh();

protected:
    void startRefresh();
    void stopRefresh();

private slots:
    void doSearch(const QString& keyword);
    void doRealSearch(bool load_more = false);
    void loadMoreSearchResults();

    void onDoubleClicked(const QModelIndex& index);

    void onSearchSuccess(const std::vector<FileSearchResult>& results,
                         bool is_loading_more,
                         bool has_more);
    void onSearchFailed(const ApiError& error);

private:
    void createSearchView();
    void createLoadingView();
    void createLoadingFailedView();
    void showLoadingView();

    bool eventFilter(QObject *obj, QEvent *event);

private:
    qint64 last_modified_;
    QTimer *search_timer_;
    FileSearchRequest *request_;

    QWidget *waiting_view_;
    QWidget *loading_view_;
    QWidget *loading_failed_view_;
    QWidget *logout_view_;

    QLabel *loading_failed_text_;
    LoadMoreButton *load_more_btn_;

    SearchBar *line_edit_;

    SearchResultItemDelegate *search_delegate_;
    SearchResultListView *search_view_;
    SearchResultListModel *search_model_;

    int nth_page_;
};
#endif // SEAFILE_CLIENT_UI_SEARCH_TAB_HSEAF
