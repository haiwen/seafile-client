#include "data-mgr.h"
#include <QDir>

#include "configurator.h"
#include "seafile-applet.h"
#include "file-browser-requests.h"

const int kLRUTTL = 60;

DataManager::DataManager(const Account &account, const ServerRepo &repo)
    : account_(account), repo_(repo)
{
    req_ = NULL;
}

DataManager::~DataManager()
{
    //nothing
}

void DataManager::getDirents(const QString& path,
                             bool force_update)
{
    QString dir_id;

    LRUCache<QString, QList<SeafDirent> > *path_cache;

    if (!repo_cache_.contains(repo_.id)) {
        path_cache = new LRUCache<QString, QList<SeafDirent> >(kLRUTTL);
        repo_cache_.insert(repo_.id, path_cache);
    } else
        path_cache = repo_cache_[repo_.id];

    if (!force_update && path_cache->contains(path)) {
        emit getDirentsSuccess(*(*path_cache)[path]);
        return;
    }

    if (req_) {
        delete req_;
    }

    req_ = new GetDirentsRequest(account_, repo_.id, path, dir_id);
    connect(req_, SIGNAL(success(const QString&, const QList<SeafDirent>&)),
            this, SLOT(onGetDirentsSuccess(const QString&, const QList<SeafDirent>&)));
    connect(req_, SIGNAL(failed(const ApiError&)),
            this, SIGNAL(getDirentsFailed(const ApiError&)));
    req_->send();
}

void DataManager::onGetDirentsSuccess(const QString &dir_id,
                                      const QList<SeafDirent> &dirents)
{
    Q_UNUSED(dir_id);
    LRUCache<QString, QList<SeafDirent> > *path_cache;
    QList<SeafDirent> *pdirents = new QList<SeafDirent>(dirents);
    path_cache = repo_cache_[req_->repoId()];
    path_cache->insert(req_->path(), pdirents);
    emit getDirentsSuccess(*pdirents);
}

void DataManager::onGetDirentsFailed(const ApiError& error)
{
    // nothing
    emit getDirentsFailed(error);
}
