#ifndef SEAFILE_CLIENT_REPO_DETAILS_VIEW_H
#define SEAFILE_CLIENT_REPO_DETAILS_VIEW_H

#include <QWidget>
#include "ui_repo-details-view.h"

class RepoDetailsView : public QWidget,
                        public Ui::RepoDetailsView
{
    Q_OBJECT

public:
    RepoDetailsView(QWidget *parent=0);

private:
    Q_DISABLE_COPY(RepoDetailsView)
};


#endif  // SEAFILE_CLIENT_REPO_DETAILS_VIEW_H
