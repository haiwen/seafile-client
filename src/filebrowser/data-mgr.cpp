#include "data-mgr.h"
#include <QDir>

#include "lrucache.h"
#include "configurator.h"
#include "seafile-applet.h"
#include "file-browser-requests.h"

const int kLRUTTL = 60;

DataManager::DataManager(const Account &account, const ServerRepo &repo)
    : account_(account), repo_(repo), path_cache_(new LRUCache<QString, QList<SeafDirent> >(kLRUTTL)), req_(NULL)
{
}

DataManager::~DataManager()
{
    if (req_)
        delete req_;
    delete path_cache_;
}

void DataManager::getDirents(const QString& path,
                             bool force_update)
{
    QString dir_id;

    if (!force_update && path_cache_->contains(path)) {
        //method object will not remove timed out item
        emit getDirentsSuccess(*(path_cache_->object(path)));
        return;
    }

    if (req_)
        delete req_;

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
    QList<SeafDirent> *pdirents = new QList<SeafDirent>(dirents);
    emit getDirentsSuccess(*pdirents);
    //path_cache_ now owns pdirents!
    path_cache_->insert(req_->path(), pdirents);
}

void DataManager::onGetDirentsFailed(const ApiError& error)
{
    // nothing
    emit getDirentsFailed(error);
}
