#include <QDir>
#include <QImage>
#include <QQueue>
#include <QHash>
#include <QTimer>
#include <QDateTime>

#include "../seafile-applet.h"
#include "../configurator.h"
#include "../account-mgr.h"
#include "../api/requests.h"
#include "../utils/paint-utils.h"
#include "../utils/utils.h"
#include "../utils/file-utils.h"

#include "thumbnail-service.h"

static const int kCheckPendingInterval = 1000; // 1s
static const qint64 kExpireTimeIntevalMsec = 300 * 1000; // 5min

struct PendingRequestInfo {
    int last_wait;
    int time_to_wait;

    void backoff() {
        last_wait = qMax(last_wait, 1) * 2;
        time_to_wait = last_wait;
    }

    bool isReady() {
        return time_to_wait == 0;
    }

    void tick() {
        time_to_wait = qMax(0, time_to_wait - 1);
    }
};

struct ThumbnailKey {
    QString repo_id;
    QString path;
    uint size;

    bool operator == (const ThumbnailKey &key) const {
        return repo_id == key.repo_id &&
               path == key.path &&
	       size == key.size;
    }
};

uint qHash(const ThumbnailKey &key) {
	QString size;
	size.setNum(key.size);
	const QString key_str = key.repo_id + key.path + size;
	return qHash(key_str);
}

class PendingThumbnailRequestQueue
{
public:
    PendingThumbnailRequestQueue() {};

    void enqueue(const QString& repo_id, const QString& path, uint size) {
        ThumbnailKey key;
	key.repo_id = repo_id;
	key.path = path;
	key.size = size;

        if (q_.contains(key)) {
            return;
        }
        // if we have set an expire time, and we haven't reached it yet
        if (expire_time_.contains(key) &&
            QDateTime::currentMSecsSinceEpoch() <= expire_time_[key]) {
            return;
        }
        // update expire time
        expire_time_[key] = QDateTime::currentMSecsSinceEpoch() + kExpireTimeIntevalMsec;

        q_.enqueue(key);
    }

    void enqueueAndBackoff(const QString& repo_id, const QString& path, uint size) {
        ThumbnailKey key;
	key.repo_id = repo_id;
	key.path = path;
	key.size = size;

        PendingRequestInfo& info = wait_[key];
        info.backoff();

        enqueue(repo_id, path, size);
    }

    void clearWait(const QString& repo_id, const QString& path, uint size) {
        ThumbnailKey key;
	key.repo_id = repo_id;
	key.path = path;
	key.size = size;

        wait_.remove(key);
    }

    void tick() {
        if (q_.isEmpty())
	    return;

        QListIterator<ThumbnailKey> iter(q_);

        while (iter.hasNext()) {
	    ThumbnailKey key;
       	    key.repo_id = iter.peekNext().repo_id;
	    key.path = iter.peekNext().path;    
	    key.size = iter.next().size;

            if (wait_.contains(key)) {
                PendingRequestInfo& info = wait_[key];
                info.tick();
            }
        }
    }

    ThumbnailKey dequeue() {
        ThumbnailKey key, key_default;
	key_default.repo_id = QString();
	key_default.path = QString();
	key_default.size = 0;

        int i = 0, n = q_.size();
        while (i++ < n) {
            if (q_.isEmpty()) {
                return key_default;
            }

            key = q_.dequeue();

            PendingRequestInfo info = wait_.value(key);
            if (info.isReady()) {
                return key;
            } else {
                q_.enqueue(key);
            }
        }

        return key_default;
    }

    void reset() {
        q_.clear();
        wait_.clear();
        expire_time_.clear();
    }

private:
    QQueue<ThumbnailKey> q_;

    QHash<ThumbnailKey, PendingRequestInfo> wait_;
    QHash<ThumbnailKey, qint64> expire_time_;
};

ThumbnailService* ThumbnailService::singleton_ = NULL;

ThumbnailService* ThumbnailService::instance()
{
    if (singleton_ == NULL) {
        static ThumbnailService instance;
        singleton_ = &instance;
    }

    return singleton_;
}


ThumbnailService::ThumbnailService(QObject *parent)
    : QObject(parent), 
      get_thumbnail_req_(NULL)
{
    queue_ = new PendingThumbnailRequestQueue;

    timer_ = new QTimer(this);

    connect(timer_, SIGNAL(timeout()), this, SLOT(checkPendingRequests()));

}

void ThumbnailService::start()
{
    timer_->start(kCheckPendingInterval);
}

// check in-memory-cache
QPixmap ThumbnailService::loadThumbnailFromLocal(const QString& repo_id, 
                                                 const QString& path,
						 uint size)
{
    QPixmap ret;

    QString key = getPixmapCacheKey(repo_id, path, size);
    if (cache_.find(key, &ret)) {
        return ret;
    }
    
    return ret;
}

QString ThumbnailService::getPixmapCacheKey(const QString& repo_id, const QString& path, uint size)
{
    const Account& account = seafApplet->accountManager()->accounts().front();
    QString size_str;
    size_str.setNum(size);
    return QDir(thumbnails_dir_).filePath(::md5(account.serverUrl.host()
			                        + account.username
                                                + repo_id 
                                                + path
						+ size_str));
}

void ThumbnailService::fetchImageFromServer(const QString& repo_id, 
                                            const QString& path,
					    uint size)
{
    if (get_thumbnail_req_) {
        if (repo_id == get_thumbnail_req_->repoId() &&
            path == get_thumbnail_req_->path() &&
	    size == get_thumbnail_req_->size()) {
            return;
        }
        queue_->enqueue(repo_id, path, size);
        return;
    }

    const Account& account = seafApplet->accountManager()->accounts().front();

    get_thumbnail_req_ = new GetThumbnailRequest(account, 
                                                 repo_id, 
                                                 path, 
                                                 size);

    connect(get_thumbnail_req_, SIGNAL(success(const QPixmap&)),
            this, SLOT(onGetThumbnailSuccess(const QPixmap&)));
    connect(get_thumbnail_req_, SIGNAL(failed(const ApiError&)),
            this, SLOT(onGetThumbnailFailed(const ApiError&)));

    get_thumbnail_req_->send();
}

void ThumbnailService::onGetThumbnailSuccess(const QPixmap& img)
{
    const QString repo_id = get_thumbnail_req_->repoId();
    const QString path_in_repo = get_thumbnail_req_->path();
    const uint size = get_thumbnail_req_->size();
    
    QString key = getPixmapCacheKey(repo_id, path_in_repo, size);

    cache_.insert(key, img);

    emit thumbnailUpdated(img, path_in_repo);

    get_thumbnail_req_->deleteLater();
    get_thumbnail_req_ = NULL;

    queue_->clearWait(repo_id, path_in_repo, size);
}

void ThumbnailService::onGetThumbnailFailed(const ApiError& error)
{
    const QString repo_id = get_thumbnail_req_->repoId();
    const QString path = get_thumbnail_req_->path();
    const uint size = get_thumbnail_req_->size();

    get_thumbnail_req_->deleteLater();
    get_thumbnail_req_ = NULL;

    queue_->enqueueAndBackoff(repo_id, path, size);
}

QPixmap ThumbnailService::getThumbnail(const QString& repo_id, 
                                       const QString& path,
				       const uint size)
{
    QPixmap img = loadThumbnailFromLocal(repo_id, path, size);

    // update all thumbnails if img is null
    if (img.isNull()) {
        if (!get_thumbnail_req_ || 
	    get_thumbnail_req_->repoId() != repo_id || 
	    get_thumbnail_req_->path() != path ||
	    get_thumbnail_req_->size() != size) {
            queue_->enqueue(repo_id, path, size);
        }
    }
    if (img.isNull()) {
        return QPixmap(devicePixelRatio() > 1 ? ":/images/files_v2/file_image@2x.png" :":/images/files_v2/file_image.png");
    } else {
        return img;
    }
}

void ThumbnailService::checkPendingRequests()
{
    queue_->tick();

    if (get_thumbnail_req_ != NULL) {
        return;
    }

    ThumbnailKey key = queue_->dequeue();
    QString repo_id = key.repo_id;
    QString path = key.path;
    uint size = key.size;
    if (!repo_id.isEmpty()) {
        fetchImageFromServer(repo_id, path, size);
    }
}
