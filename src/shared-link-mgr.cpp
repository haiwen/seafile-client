#include "api/requests.h"
#include "ui/seafilelink-dialog.h"
#include "ui/sharedlink-dialog.h"
#include "ui/advanced-sharedlink-dialog.h"
#include "account-mgr.h"
#include "seafile-applet.h"
#include "utils/utils.h"

#include "shared-link-mgr.h"

SINGLETON_IMPL(SharedLinkManager)

namespace {


} // namespace

SharedLinkManager::SharedLinkManager()
{
}

SharedLinkManager::~SharedLinkManager()
{
    Q_FOREACH(SeafileApiRequest *req, reqs_)
    {
        req->deleteLater();
    }
}

void SharedLinkManager::generateShareLink(SharedLinkRequestParams& params)
{
    if (!params.is_file && !params.path_in_repo.endsWith("/")) {
        params.path_in_repo += "/";
    }

    // check previous shared link first
    getSharedLink(params);
}

void SharedLinkManager::getSharedLink(const SharedLinkRequestParams& params)
{
    if (params.internal) {
        SeafileLinkDialog *dialog = new SeafileLinkDialog(
            params.repo_id, params.account, params.path_in_repo, NULL);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->show();
        dialog->raise();
        dialog->activateWindow();
    } else {
        GetSharedLinkRequest *req = new GetSharedLinkRequest(params);
        connect(req, SIGNAL(success(const SharedLinkInfo&)),
                this, SLOT(onGetSharedLinkSuccess(const SharedLinkInfo&)));
        connect(req, SIGNAL(failed(const ApiError&)),
                this, SLOT(onGetSharedLinkFailed(const ApiError&)));
        connect(req, SIGNAL(empty()),
                this, SLOT(onGetSharedLinkEmpty()));
        req->send();
        reqs_.push_back(req);
    }
}

void SharedLinkManager::onGetSharedLinkSuccess(
    const SharedLinkInfo& shared_link_info)
{
    bool proceed = false;
    proceed = seafApplet->detailedYesOrNoBox(tr(
        "<b>Warning:</b> The shared link already exists, "
        "delete and create link anyway?"),
        "username: " + shared_link_info.username +
	"\nlink: " + shared_link_info.link +
	"\nview_cnt: " + QString::number(shared_link_info.view_cnt),
        0, true);
    if (!proceed) {
        return;
    } else {
        GetSharedLinkRequest *get_req = qobject_cast<GetSharedLinkRequest *>(sender());
        deleteSharedLink(get_req->req_params, shared_link_info.token);
    }
}

void SharedLinkManager::onGetSharedLinkFailed(const ApiError&)
{
    seafApplet->warningBox(tr("Failed to get the shared link"));
}

void SharedLinkManager::onGetSharedLinkEmpty()
{
    GetSharedLinkRequest *get_req = qobject_cast<GetSharedLinkRequest *>(sender());
    createSharedLink(get_req->req_params);
}

void SharedLinkManager::deleteSharedLink(const SharedLinkRequestParams &params,
                                         const QString &token)
{
    const QString repo_id = params.repo_id;
    DeleteSharedLinkRequest *req = new DeleteSharedLinkRequest(params, token);
    connect(req, SIGNAL(success()),
            this, SLOT(onDeleteSharedLinkSuccess()));
    connect(req, SIGNAL(failed(const ApiError&)),
            this, SLOT(onDeleteSharedLinkFailed(const ApiError&)));
    req->send();
    reqs_.push_back(req);
}

void SharedLinkManager::onDeleteSharedLinkSuccess()
{
    DeleteSharedLinkRequest *del_req = qobject_cast<DeleteSharedLinkRequest *>(sender());
    createSharedLink(del_req->req_params);
}

void SharedLinkManager::onDeleteSharedLinkFailed(const ApiError&)
{
    seafApplet->warningBox(tr("Failed to delete the shared link"));
}

void SharedLinkManager::createSharedLink(const SharedLinkRequestParams& params)
{
    if (!params.advanced) {
        CreateShareLinkRequest *req = new CreateShareLinkRequest(params);
        connect(req, SIGNAL(success(const SharedLinkInfo&)),
                this, SLOT(onCreateSharedLinkSuccess(const SharedLinkInfo&)));
        connect(req, SIGNAL(failed(const ApiError&)),
                this, SLOT(onCreateSharedLinkFailed(const ApiError&)));
        req->send();
        reqs_.push_back(req);
    } else {
        AdvancedSharedLinkDialog *dialog =
            new AdvancedSharedLinkDialog(NULL, params);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->show();
        dialog->raise();
        dialog->activateWindow();
    }
}

void SharedLinkManager::onCreateSharedLinkSuccess(const SharedLinkInfo& shared_link_info)
{
    SharedLinkDialog *dialog = new SharedLinkDialog(shared_link_info.link, NULL);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
    dialog->raise();
    dialog->activateWindow();
}

void SharedLinkManager::onCreateSharedLinkFailed(const ApiError&)
{
    seafApplet->warningBox(tr("Failed to create the shared link"));
}
