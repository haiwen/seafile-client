#include <QDir>

#include "configurator.h"
#include "seafile-applet.h"
#include "file-browser-requests.h"

#include "data-mgr.h"

DataManager::DataManager(const Account& account)
    : account_(account)
{
    req_ = 0;
    cache_ = new FileBrowserCache();
}

void DataManager::getDirents(const QString repo_id,
                             const QString& path)
{
    // TODO: real caching
    QPair<QString, std::vector<SeafDirent> > cached = cache_->getCachedDirents(repo_id, path);
    QString dir_id = cached.first;
    if (!dir_id.isEmpty()) {
        emit getDirentsSuccess(cached.second);
        return;
    }

    if (req_) {
        delete req_;
    }

    req_ = new GetDirentsRequest(account_, repo_id, path, dir_id);
    connect(req_, SIGNAL(success(const QString&, const std::vector<SeafDirent>&)),
            this, SLOT(onGetDirentsSuccess(const QString&, const std::vector<SeafDirent>&)));
    connect(req_, SIGNAL(failed(const ApiError&)),
            this, SIGNAL(getDirentsFailed(const ApiError&)));
    req_->send();
}

void DataManager::onGetDirentsSuccess(const QString& dir_id, const std::vector<SeafDirent>& dirents)
{
    cache_->saveDirents(req_->repoId(), req_->path(), dir_id, dirents);
    emit getDirentsSuccess(dirents);
}


FileBrowserCache::FileBrowserCache()
{
    QDir seafdir = seafApplet->configurator()->seafileDir();
    // TODO: setup cache
}

QPair<QString, std::vector<SeafDirent> >
FileBrowserCache::getCachedDirents(const QString repo_id,
                                   const QString& path)
{
    QPair<QString, std::vector<SeafDirent> > pair;
    return pair;
}

void FileBrowserCache::saveDirents(const QString repo_id,
                                   const QString& path,
                                   const QString& dir_id,
                                   const std::vector<SeafDirent>& dirents)
{
}
