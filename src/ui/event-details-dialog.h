#ifndef SEAFILE_CLIENT_UI_EVENT_DETAILS_DIALOG_H
#define SEAFILE_CLIENT_UI_EVENT_DETAILS_DIALOG_H

#include <QDialog>
#include "api/event.h"

class GetCommitDetailsRequest;
struct CommitDetails;
class EventDetailsTreeView;
class EventDetailsTreeModel;
class ApiError;

// Display the commit details in a simple tree view
class EventDetailsDialog : public QDialog {
    Q_OBJECT
public:
    EventDetailsDialog(const SeafEvent& event, QWidget *parent=0);

private slots:
    void updateContent(const CommitDetails& details);
    void getCommitDetailsFailed(const ApiError& error);

private:
    Q_DISABLE_COPY(EventDetailsDialog)

    void sendRequest();

    SeafEvent event_;

    GetCommitDetailsRequest *request_;

    EventDetailsTreeView *tree_;
    EventDetailsTreeModel *model_;
    QWidget *loading_view_;
};

#endif // SEAFILE_CLIENT_UI_EVENT_DETAILS_DIALOG_H
