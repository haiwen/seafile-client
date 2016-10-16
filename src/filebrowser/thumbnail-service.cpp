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
static const char *kThumbnailsDirName = "thumbnails";
static const qint64 kExpireTimeIntevalMsec = 300 * 1000; // 5min
static const int kColumnIconSize = 28;

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

    bool operator == (const ThumbnailKey &key) const {
        return repo_id == key.repo_id &&
               path == key.path;
    }
};

uint qHash(const ThumbnailKey &key) {
	const QString key_str = key.repo_id + key.path;
	return qHash(key_str);
}

class PendingThumbnailRequestQueue
{
public:
    PendingThumbnailRequestQueue() {};

    void enqueue(const QString& repo_id, const QString& path) {
        ThumbnailKey key;
	key.repo_id = repo_id;
	key.path = path;

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

    void enqueueAndBackoff(const QString& repo_id, const QString& path) {
        ThumbnailKey key;
	key.repo_id = repo_id;
	key.path = path;

        PendingRequestInfo& info = wait_[key];
        info.backoff();

        enqueue(repo_id, path);
    }

    void clearWait(const QString& repo_id, const QString& path) {
        ThumbnailKey key;
	key.repo_id = repo_id;
	key.path = path;

        wait_.remove(key);
    }

    void tick() {
        if (q_.isEmpty())
	    return;

        QListIterator<ThumbnailKey> iter(q_);

        while (iter.hasNext()) {
	    ThumbnailKey key;
       	    key.repo_id = iter.peekNext().repo_id;
	    key.path = iter.next().path;    

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
      kThumbnailSize(kColumnIconSize),
      get_thumbnail_req_(NULL)
{
    queue_ = new PendingThumbnailRequestQueue;

    timer_ = new QTimer(this);

    connect(timer_, SIGNAL(timeout()), this, SLOT(checkPendingRequests()));

}

void ThumbnailService::start()
{
    QDir seafile_dir(seafApplet->configurator()->seafileDir());
    const Account& account = seafApplet->accountManager()->accounts().front();

    const QString sub_path(::pathJoin(kThumbnailsDirName, account.username)); 
    if (!seafile_dir.mkpath(sub_path)) {
        QString err_msg = tr("Failed to create thumbnails folder");
	qWarning("%s", err_msg.toUtf8().data());
        seafApplet->errorAndExit(err_msg);
    }

    thumbnails_dir_ = seafile_dir.filePath(sub_path);

    timer_->start(kCheckPendingInterval);
}

// fist check in-memory-cache, then check saved image on disk
QPixmap ThumbnailService::loadThumbnailFromLocal(const QString& repo_id, 
                                                 const QString& path)
{
    QPixmap ret;

    QString local_path = getThumbnailFilePath(repo_id, path);
    if (cache_.find(local_path, &ret)) {
        return ret;
    }
    
    if (thumbnailFileExists(repo_id, path)) {
        ret = QPixmap(local_path);
        cache_.insert(local_path, ret);
    }

    return ret;
}

QString ThumbnailService::getThumbnailFilePath(const QString& repo_id, const QString& path)
{
    const Account& account = seafApplet->accountManager()->accounts().front();
    return QDir(thumbnails_dir_).filePath(::md5(account.serverUrl.host()
                                                + repo_id 
                                                + path));
}

void ThumbnailService::fetchImageFromServer(const QString& repo_id, 
                                            const QString& path)
{
    if (get_thumbnail_req_) {
        if (repo_id == get_thumbnail_req_->repoId() &&
            path == get_thumbnail_req_->path()) {
            return;
        }
        queue_->enqueue(repo_id, path);
        return;
    }

    const Account& account = seafApplet->accountManager()->accounts().front();

    get_thumbnail_req_ = new GetThumbnailRequest(account, 
                                                 repo_id, 
                                                 path, 
                                                 kThumbnailSize);

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

    // if no change? early return
    if (img.isNull()) {
        get_thumbnail_req_->deleteLater();
	get_thumbnail_req_ = NULL;

        queue_->clearWait(repo_id, path_in_repo);
        return;
    }

    // save image to thumbnails/ folder
    QString path = getThumbnailFilePath(repo_id, path_in_repo);
    if (!img.save(path, "PNG")) {
        qWarning("Unable to save new thumbnail file %s", path.toUtf8().data());
    }

    cache_.insert(path, img);

    emit thumbnailUpdated(img, path_in_repo);

    get_thumbnail_req_->deleteLater();
    get_thumbnail_req_ = NULL;

    queue_->clearWait(repo_id, path_in_repo);
}

void ThumbnailService::onGetThumbnailFailed(const ApiError& error)
{
    const QString repo_id = get_thumbnail_req_->repoId();
    const QString path = get_thumbnail_req_->path();
    get_thumbnail_req_->deleteLater();
    get_thumbnail_req_ = NULL;

    queue_->enqueueAndBackoff(repo_id, path);
}

QPixmap ThumbnailService::getThumbnail(const QString& repo_id, 
                                       const QString& path)
{
    QPixmap img = loadThumbnailFromLocal(repo_id, path);

    // update all thumbnails if img is null
    if (img.isNull()) {
        if (!get_thumbnail_req_ || 
	    get_thumbnail_req_->repoId() != repo_id || 
	    get_thumbnail_req_->path() != path) {
            queue_->enqueue(repo_id, path);
        }
    }
    if (img.isNull()) {
        return QPixmap(devicePixelRatio() > 1 ? ":/images/files_v2/file_image@2x.png" :":/images/files_v2/file_image.png");
    } else {
        return img;
    }
}

bool ThumbnailService::thumbnailFileExists(const QString& repo_id, const QString& path)
{
    QString local_path = getThumbnailFilePath(repo_id, path);
    bool ret = QFileInfo(local_path).exists();

    return ret;
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
    if (!repo_id.isEmpty()) {
        fetchImageFromServer(repo_id, path);
    }
}
