#ifndef SEAFILE_CLIENT_UI_EVENT_DETAILS_DIALOG_H
#define SEAFILE_CLIENT_UI_EVENT_DETAILS_DIALOG_H

#include <QDialog>
#include "api/event.h"

class GetCommitDetailsRequest;
class CommitDetails;
class EventDetailsTreeView;
class EventDetailsTreeModel;

// Display the commit details in a simple tree view
class EventDetailsDialog : public QDialog {
    Q_OBJECT
public:
    EventDetailsDialog(const SeafEvent& event, QWidget *parent=0);

private slots:
    void updateContent(const CommitDetails& details);

private:
    Q_DISABLE_COPY(EventDetailsDialog)

    SeafEvent event_;

    GetCommitDetailsRequest *get_details_req_;

    EventDetailsTreeView *tree_;
    EventDetailsTreeModel *model_;
    QWidget *loading_view_;
};

#endif // SEAFILE_CLIENT_UI_EVENT_DETAILS_DIALOG_H
