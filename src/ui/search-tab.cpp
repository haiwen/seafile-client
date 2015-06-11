#include "ui/search-tab.h"
#include "ui/search-tab-items.h"

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
#include "utils/file-utils.h"

namespace {
const int kMinimumKeywordLength = 3;
const int kInputDelayInterval = 300;
const char *kLoadingFailedLabelName = "loadingFailedText";

enum {
    INDEX_WAITING_VIEW = 0,
    INDEX_LOADING_VIEW,
    INDEX_LOADING_FAILED_VIEW,
    INDEX_SEARCH_VIEW,
};

} // anonymous namespace

SearchTab::SearchTab(QWidget *parent)
    : TabView(parent), last_modified_(0), request_(NULL)
{
    createSearchView();
    createLoadingView();
    createLoadingFailedView();

    mStack->insertWidget(INDEX_WAITING_VIEW, waiting_view_);
    mStack->insertWidget(INDEX_LOADING_VIEW, loading_view_);
    mStack->insertWidget(INDEX_LOADING_FAILED_VIEW, loading_failed_view_);
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
    line_edit_ = new QLineEdit;
    line_edit_->setPlaceholderText(tr("Search Files..."));
    line_edit_->setObjectName("searchInput");
#ifdef Q_OS_MAC
    line_edit_->setAttribute(Qt::WA_MacShowFocusRect, 0);
#endif
#if (QT_VERSION >= QT_VERSION_CHECK(5, 2, 0))
    line_edit_->setClearButtonEnabled(true);
#endif
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
        scale_factor = painter.device()->devicePixelRatio();
#endif // QT5

        QPixmap image(QIcon(":/images/main-panel/search-background.png").pixmap(size).scaled(scale_factor * size));

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
        image.setDevicePixelRatio(scale_factor);
#endif // QT5

        painter.drawPixmap(rect.topLeft(), image);

        return true;
    };
    return QObject::eventFilter(obj, event);
}

void SearchTab::refresh()
{
    if (!line_edit_->text().isEmpty()) {
        last_modified_ = 1;
        doRealSearch();
    }
}

void SearchTab::startRefresh()
{
    search_timer_->start();
    refresh();
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

void SearchTab::doRealSearch()
{
    // not modified
    if (last_modified_ == 0)
        return;
    // modified too fast
    if (QDateTime::currentMSecsSinceEpoch() - last_modified_ <= kInputDelayInterval)
        return;

    if (!seafApplet->accountManager()->hasAccount())
        return;
    if (request_) {
        request_->deleteLater();
        request_ = NULL;
    }
    mStack->setCurrentIndex(INDEX_LOADING_VIEW);
    request_ = new FileSearchRequest(seafApplet->accountManager()->accounts().front(), line_edit_->text());
    connect(request_, SIGNAL(success(const std::vector<FileSearchResult>&)),
            this, SLOT(onSearchSuccess(const std::vector<FileSearchResult>&)));
    connect(request_, SIGNAL(failed(const ApiError&)),
            this, SLOT(onSearchFailed(const ApiError&)));

    request_->send();

    // reset
    last_modified_ = 0;
}

void SearchTab::onSearchSuccess(const std::vector<FileSearchResult>& results)
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

    search_model_->clear();
    search_model_->addItems(items);

    mStack->setCurrentIndex(INDEX_SEARCH_VIEW);
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
