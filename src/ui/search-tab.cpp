#include <QtGlobal>

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
#include <QtWidgets>
#else
#include <QtGui>
#endif

#include "api/requests.h"
#include "seafile-applet.h"
#include "account-mgr.h"
#include "repo-service.h"
#include "loading-view.h"
#include "logout-view.h"
#include "utils/file-utils.h"
#include "utils/paint-utils.h"
#include "ui/search-bar.h"

#include "ui/search-tab.h"
#include "ui/search-tab-items.h"

namespace {
const int kMinimumKeywordLength = 3;
const int kInputDelayInterval = 300;
const char *kLoadingFailedLabelName = "loadingFailedText";

enum {
    INDEX_WAITING_VIEW = 0,
    INDEX_LOADING_VIEW,
    INDEX_LOADING_FAILED_VIEW,
    INDEX_LOGOUT_VIEW,
    INDEX_SEARCH_VIEW,
};

} // anonymous namespace

SearchTab::SearchTab(QWidget *parent)
    : TabView(parent), last_modified_(0), request_(NULL), nth_page_(1)
{
    createSearchView();
    createLoadingView();
    createLoadingFailedView();

    //createLogoutView
    logout_view_ = new LogoutView;
    static_cast<LogoutView*>(logout_view_)->setQssStyleForTab();

    mStack->insertWidget(INDEX_WAITING_VIEW, waiting_view_);
    mStack->insertWidget(INDEX_LOADING_VIEW, loading_view_);
    mStack->insertWidget(INDEX_LOADING_FAILED_VIEW, loading_failed_view_);
    mStack->insertWidget(INDEX_LOGOUT_VIEW, logout_view_);
    mStack->insertWidget(INDEX_SEARCH_VIEW, search_view_);

    connect(line_edit_, SIGNAL(textChanged(const QString&)),
            this, SLOT(doSearch(const QString&)));

    connect(search_view_, SIGNAL(doubleClicked(const QModelIndex&)),
            this, SLOT(onDoubleClicked(const QModelIndex&)));

    search_timer_ = new QTimer(this);
    connect(search_timer_, SIGNAL(timeout()), this, SLOT(doRealSearch()));
    search_timer_->start(kInputDelayInterval);
}

SearchTab::~SearchTab()
{
    stopRefresh();
    delete search_model_;
}

void SearchTab::reset()
{
    stopRefresh();
    line_edit_->setText("");
    mStack->setCurrentIndex(INDEX_WAITING_VIEW);
}

void SearchTab::createSearchView()
{
    QVBoxLayout *layout = (QVBoxLayout*)this->layout();
    line_edit_ = new SearchBar;
    line_edit_->setPlaceholderText(tr("Search files"));
    layout->insertWidget(0, line_edit_);

    waiting_view_ = new QWidget;
    waiting_view_->installEventFilter(this);

    search_view_ = new SearchResultListView;
    search_view_->setObjectName("searchResult");
#ifdef Q_OS_MAC
    search_view_->setAttribute(Qt::WA_MacShowFocusRect, 0);
#endif
    search_model_ = new SearchResultListModel;
    search_view_->setModel(search_model_);

    search_delegate_ = new SearchResultItemDelegate;

    delete search_view_->itemDelegate();
    search_view_->setItemDelegate(search_delegate_);
}

void SearchTab::createLoadingView()
{
    loading_view_ = new LoadingView;
    static_cast<LoadingView*>(loading_view_)->setQssStyleForTab();
}

void SearchTab::createLoadingFailedView()
{
    loading_failed_view_ = new QWidget(this);

    QVBoxLayout *layout = new QVBoxLayout;
    loading_failed_view_->setLayout(layout);

    loading_failed_text_ = new QLabel;
    loading_failed_text_->setObjectName(kLoadingFailedLabelName);
    loading_failed_text_->setAlignment(Qt::AlignCenter);
    QString link = QString("<a style=\"color:#777\" href=\"#\">%1</a>").arg(tr("retry"));
    QString label_text = tr("Failed to search<br/>"
                            "Please %1").arg(link);
    loading_failed_text_->setText(label_text);

    connect(loading_failed_text_, SIGNAL(linkActivated(const QString&)),
            this, SLOT(refresh()));

    layout->addWidget(loading_failed_text_);
}

void SearchTab::showLoadingView()
{
    mStack->setCurrentIndex(INDEX_LOADING_VIEW);
}

bool SearchTab::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == waiting_view_ && event->type() == QEvent::Paint) {
        QPainter painter(waiting_view_);

        QPaintEvent *ev = (QPaintEvent*)event;
        const QSize size(72, 72);
        const int x = ev->rect().width() / 2 - size.width() / 2;
        const int y = ev->rect().height() / 2 - size.height() / 2;
        QRect rect(QPoint(x, y), size);

        // get the device pixel radio from current painter device
        int scale_factor = 1;
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
        scale_factor = globalDevicePixelRatio();
#endif // QT5

        QPixmap image = QIcon(":/images/main-panel/search-background.png").pixmap(size);
        painter.drawPixmap(rect, image);

        return true;
    };
    return QObject::eventFilter(obj, event);
}

void SearchTab::refresh()
{
    if (!seafApplet->accountManager()->currentAccount().isValid()) {
        mStack->setCurrentIndex(INDEX_LOGOUT_VIEW);
        return;
    }
    if (!line_edit_->text().isEmpty()) {
        last_modified_ = 1;
        doRealSearch();
    }
}

void SearchTab::startRefresh()
{
    search_timer_->start();
}

void SearchTab::stopRefresh()
{
    search_timer_->stop();
    if (request_) {
        request_->deleteLater();
        request_ = NULL;
    }
}

void SearchTab::doSearch(const QString& keyword)
{
    // make it search utf-8 charcters
    if (keyword.toUtf8().size() < kMinimumKeywordLength) {
        mStack->setCurrentIndex(INDEX_WAITING_VIEW);
        return;
    }

    // save for doRealSearch
    last_modified_ = QDateTime::currentMSecsSinceEpoch();
}

void SearchTab::doRealSearch(bool load_more)
{
    if (!load_more) {
        // not modified
        if (last_modified_ == 0)
            return;
        // modified too fast
        if (QDateTime::currentMSecsSinceEpoch() - last_modified_ <= kInputDelayInterval)
            return;
    }

    const Account& account = seafApplet->accountManager()->currentAccount();

    if (!account.isValid())
        return;
    if (request_) {
        // request_->abort();
        request_->deleteLater();
        request_ = NULL;
    }

    if (!load_more) {
        nth_page_ = 1;
        mStack->setCurrentIndex(INDEX_LOADING_VIEW);
    } else {
        nth_page_++;
    }

    request_ = new FileSearchRequest(account, line_edit_->text(), nth_page_);
    connect(request_, SIGNAL(success(const std::vector<FileSearchResult>&, bool, bool)),
            this, SLOT(onSearchSuccess(const std::vector<FileSearchResult>&, bool, bool)));
    connect(request_, SIGNAL(failed(const ApiError&)),
            this, SLOT(onSearchFailed(const ApiError&)));

    request_->send();

    // reset
    last_modified_ = 0;
}

void SearchTab::onSearchSuccess(const std::vector<FileSearchResult>& results,
                                bool is_loading_more,
                                bool has_more)
{
    std::vector<QListWidgetItem*> items;

    for (unsigned i = 0; i < results.size(); ++i) {
        QListWidgetItem *item = new QListWidgetItem(results[i].name);
        if (results[i].fullpath.endsWith("/"))
            item->setIcon(QIcon(getIconByFolder()));
        else
            item->setIcon(QIcon(getIconByFileName(results[i].name)));
        item->setData(Qt::UserRole, QVariant::fromValue(results[i]));
        items.push_back(item);
    }

    mStack->setCurrentIndex(INDEX_SEARCH_VIEW);

    const QModelIndex first_new_item = search_model_->updateSearchResults(items, is_loading_more, has_more);
    if (first_new_item.isValid()) {
        search_view_->scrollTo(first_new_item);
    }

    if (has_more) {
        load_more_btn_ = new LoadMoreButton;
        connect(load_more_btn_, SIGNAL(clicked()),
                this, SLOT(loadMoreSearchResults()));
        search_view_->setIndexWidget(
            search_model_->loadMoreIndex(), load_more_btn_);
    }
}

void SearchTab::loadMoreSearchResults()
{
    doRealSearch(true);
}

void SearchTab::onSearchFailed(const ApiError& error)
{
    mStack->setCurrentIndex(INDEX_LOADING_FAILED_VIEW);
}

void SearchTab::onDoubleClicked(const QModelIndex& index)
{
    FileSearchResult result = search_model_->data(index, Qt::UserRole).value<FileSearchResult>();
    if (result.name.isEmpty() || result.fullpath.isEmpty())
        return;

    RepoService::instance()->openLocalFile(result.repo_id, result.fullpath);
}
