#include <QtGlobal>

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
#include <QtWidgets>
#else
#include <QtGui>
#endif
#include <QTimer>
#include <QFileInfo>
#include <QIcon>
#include <QStackedWidget>

#include "seafile-applet.h"
#include "account-mgr.h"
#include "api/requests.h"
#include "api/starred-file.h"
#include "loading-view.h"
#include "logout-view.h"
#include "starred-files-list-view.h"
#include "starred-files-list-model.h"
#include "starred-file-item-delegate.h"

#include "starred-files-tab.h"

namespace {

const int kRefreshInterval = 1000 * 60 * 5; // 5 min
const char *kLoadingFaieldLabelName = "loadingFailedText";
const char *kEmptyViewLabelName = "emptyText";

enum {
    INDEX_LOADING_VIEW = 0,
    INDEX_LOADING_FAILED_VIEW,
    INDEX_EMPTY_VIEW,
    INDEX_LOGOUT_VIEW,
    INDEX_FILES_VIEW
};

}

StarredFilesTab::StarredFilesTab(QWidget *parent)
    : TabView(parent),
      in_refresh_(false)
{
    createStarredFilesListView();
    createLoadingView();
    createLoadingFailedView();

    //createLogoutView
    logout_view_ = new LogoutView;
    static_cast<LogoutView*>(logout_view_)->setQssStyleForTab();

    createEmptyView();

    mStack->insertWidget(INDEX_LOADING_VIEW, loading_view_);
    mStack->insertWidget(INDEX_LOADING_FAILED_VIEW, loading_failed_view_);
    mStack->insertWidget(INDEX_EMPTY_VIEW, empty_view_);
    mStack->insertWidget(INDEX_LOGOUT_VIEW, logout_view_);
    mStack->insertWidget(INDEX_FILES_VIEW, files_list_view_);

    refresh_timer_ = new QTimer(this);
    connect(refresh_timer_, SIGNAL(timeout()), this, SLOT(refresh()));

    get_starred_files_req_ = NULL;
    get_starred_items_req_ = NULL;

    refresh();
}

void StarredFilesTab::createStarredFilesListView()
{
    files_list_view_ = new StarredFilesListView;
    files_list_model_ = new StarredFilesListModel;

    files_list_view_->setModel(files_list_model_);
    files_list_view_->setItemDelegate(new StarredFileItemDelegate);
}

void StarredFilesTab::createLoadingView()
{
    loading_view_ = new LoadingView;
    static_cast<LoadingView*>(loading_view_)->setQssStyleForTab();
}

void StarredFilesTab::createLoadingFailedView()
{
    loading_failed_view_ = new QWidget(this);

    QVBoxLayout *layout = new QVBoxLayout;
    loading_failed_view_->setLayout(layout);

    QLabel *label = new QLabel;
    label->setObjectName(kLoadingFaieldLabelName);
    QString link = QString("<a style=\"color:#777\" href=\"#\">%1</a>").arg(tr("retry"));
    QString label_text = tr("Failed to get starred files information<br/>"
                            "Please %1").arg(link);
    label->setText(label_text);
    label->setAlignment(Qt::AlignCenter);

    connect(label, SIGNAL(linkActivated(const QString&)),
            this, SLOT(refresh()));

    layout->addWidget(label);
}

void StarredFilesTab::createEmptyView()
{
    empty_view_ = new QWidget(this);
    empty_view_->setObjectName("EmptyPlaceHolder");

    QVBoxLayout *layout = new QVBoxLayout;
    empty_view_->setLayout(layout);

    QLabel *label = new QLabel;
    label->setObjectName(kEmptyViewLabelName);
    QString label_text = tr("You have no starred files yet.");
    label->setText(label_text);
    label->setAlignment(Qt::AlignCenter);

    layout->addWidget(label);
}

void StarredFilesTab::refresh()
{
    if (!seafApplet->accountManager()->hasAccount() ||
        !seafApplet->accountManager()->accounts().front().isValid()) {
        mStack->setCurrentIndex(INDEX_LOGOUT_VIEW);
        return;
    }
    if (in_refresh_) {
        return;
    }

    in_refresh_ = true;

    showLoadingView();
    //AccountManager *account_mgr = seafApplet->accountManager();

    const std::vector<Account>& accounts = seafApplet->accountManager()->accounts();
    if (!seafApplet->accountManager()->hasAccount()) {
        in_refresh_ = false;
        return;
    }

    // server version 7.0.0 begin to support starred file dir repo
    bool is_use_get_starred_item_api = seafApplet->accountManager()->currentAccount().isAtLeastVersion(7, 0, 0);

    if (get_starred_files_req_) {
        get_starred_files_req_->deleteLater();
    }

    if (get_starred_items_req_) {
        get_starred_items_req_->deleteLater();
    }

    if (!is_use_get_starred_item_api) {
        get_starred_files_req_ = new GetStarredFilesRequest(accounts[0]);
        connect(get_starred_files_req_, SIGNAL(success(const std::vector<StarredItem>&)),
                this, SLOT(refreshStarredFiles(const std::vector<StarredItem>&)));
        connect(get_starred_files_req_, SIGNAL(failed(const ApiError&)),
                this, SLOT(refreshStarredFilesFailed(const ApiError&)));
        get_starred_files_req_->send();
    } else {
        get_starred_items_req_ = new GetStarredFilesRequestV2(accounts[0]);
        connect(get_starred_items_req_, SIGNAL(success(const std::vector<StarredItem>&)),
                this, SLOT(refreshStarredFilesV2(const std::vector<StarredItem>&)));
        connect(get_starred_items_req_, SIGNAL(failed(const ApiError&)),
                this, SLOT(refreshStarredFilesFailed(const ApiError&)));
        get_starred_items_req_->send();
    }

}

void StarredFilesTab::refreshStarredFiles(const std::vector<StarredItem>& files)
{
    in_refresh_ = false;

    get_starred_files_req_->deleteLater();
    get_starred_files_req_ = NULL;

    files_list_model_->setFiles(files);
    if (files.empty()) {
        mStack->setCurrentIndex(INDEX_EMPTY_VIEW);
    } else {
        mStack->setCurrentIndex(INDEX_FILES_VIEW);
    }
}

void StarredFilesTab::refreshStarredFilesV2(const std::vector<StarredItem>& files)
{
    in_refresh_ = false;

    get_starred_items_req_->deleteLater();
    get_starred_items_req_ = NULL;

    files_list_model_->setFiles(files);
    if (files.empty()) {
        mStack->setCurrentIndex(INDEX_EMPTY_VIEW);
    } else {
        mStack->setCurrentIndex(INDEX_FILES_VIEW);
    }
}

void StarredFilesTab::refreshStarredFilesFailed(const ApiError& error)
{
    qDebug("failed to refresh starred files");
    in_refresh_ = false;

    if (mStack->currentIndex() == INDEX_LOADING_VIEW) {
        mStack->setCurrentIndex(INDEX_LOADING_FAILED_VIEW);
    }
}

void StarredFilesTab::showLoadingView()
{
    mStack->setCurrentIndex(INDEX_LOADING_VIEW);
}

void StarredFilesTab::startRefresh()
{
    refresh_timer_->start(kRefreshInterval);
}

void StarredFilesTab::stopRefresh()
{
    refresh_timer_->stop();
}
