#include <QtGui>
#include <QVBoxLayout>
#include <QStackedLayout>

#include "seafile-applet.h"
#include "account-mgr.h"
#include "utils/widget-utils.h"
#include "api/requests.h"
#include "api/commit-details.h"
#include "event-details-tree.h"

#include "event-details-dialog.h"

namespace {

enum {
    INDEX_LOADING_VIEW = 0,
    INDEX_DETAILS_VIEW
};

} // namespace

EventDetailsDialog::EventDetailsDialog(const SeafEvent& event, QWidget *parent)
    : QDialog(parent),
      event_(event)
{
    setWindowTitle(tr("Modification Details"));
    setWindowIcon(QIcon(":/images/seafile.png"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    QStackedLayout *layout = new QStackedLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    setLayout(layout);

    loading_view_ = ::newLoadingView();
    layout->addWidget(loading_view_);

    tree_ = new EventDetailsTreeView(event);
    model_ = new EventDetailsTreeModel(event);
    tree_->setModel(model_);

    layout->addWidget(tree_);

    const Account& account = seafApplet->accountManager()->currentAccount();

    get_details_req_ = new GetCommitDetailsRequest(account, event.repo_id, event.commit_id);
    connect(get_details_req_, SIGNAL(success(const CommitDetails&)),
            this, SLOT(updateContent(const CommitDetails&)));
    get_details_req_->send();
}

void EventDetailsDialog::updateContent(const CommitDetails& details)
{
    QStackedLayout *layout = (QStackedLayout *)this->layout();
    layout->setCurrentIndex(INDEX_DETAILS_VIEW);

    model_->setCommitDetails(details);

    tree_->expandAll();
}
