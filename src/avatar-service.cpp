#include <QDir>
#include <QImage>
#include <QQueue>
#include <QHash>
#include <QTimer>

#include "seafile-applet.h"
#include "configurator.h"
#include "account-mgr.h"
#include "api/requests.h"
#include "utils/paint-utils.h"
#include "utils/utils.h"

#include "avatar-service.h"

namespace {

const int kCheckPendingInterval = 1000; // 1s
const char *kAvatarsDirName = "avatars";

} // namespace

struct PendingRequestInfo {
    int last_wait;
    int time_to_wait;

    void backoff() {
        last_wait = qMax(last_wait, 1) * 2;
        printf ("backoff: last_wait = %d\n", last_wait);
        time_to_wait = last_wait;
    }

    bool isReady() {
        return time_to_wait == 0;
    }

    void tick() {
        time_to_wait = qMax(0, time_to_wait - 1);
    }
};

class PendingAvatarRequestQueue
{
public:
    PendingAvatarRequestQueue() {};

    void enqueue(const QString& email) {
        if (q_.contains(email)) {
            return;
        }

        q_.enqueue(email);
    }

    void enqueueAndBackoff(const QString& email) {
        PendingRequestInfo& info = wait_[email];
        info.backoff();

        enqueue(email);
    }

    void clearWait(const QString& email) {
        wait_.remove(email);
    }

    void tick() {
        QListIterator<QString> iter(q_);

        while (iter.hasNext()) {
            QString email = iter.next();
            if (wait_.contains(email)) {
                PendingRequestInfo& info = wait_[email];
                info.tick();
            }
        }
    }

    QString dequeue() {
        int i = 0, n = q_.size();
        while (i++ < n) {
            if (q_.isEmpty()) {
                return QString();
            }

            QString email = q_.dequeue();

            PendingRequestInfo info = wait_.value(email);
            if (info.isReady()) {
                return email;
            } else {
                q_.enqueue(email);
            }
        }

        return QString();
    }

    void reset() {
        q_.clear();
        wait_.clear();
    }

private:
    QQueue<QString> q_;

    QHash<QString, PendingRequestInfo> wait_;
};

AvatarService* AvatarService::singleton_;

AvatarService* AvatarService::instance()
{
    if (singleton_ == NULL) {
        static AvatarService instance;
        singleton_ = &instance;
    }

    return singleton_;
}


AvatarService::AvatarService(QObject *parent)
    : QObject(parent)
{
    get_avatar_req_ = 0;

    queue_ = new PendingAvatarRequestQueue;

    timer_ = new QTimer(this);

    connect(timer_, SIGNAL(timeout()), this, SLOT(checkPendingRequests()));

    connect(seafApplet->accountManager(), SIGNAL(accountsChanged()),
            this, SLOT(onAccountChanged()));
}

void AvatarService::start()
{
    QDir seafile_dir(seafApplet->configurator()->seafileDir());

    if (!seafile_dir.mkpath(kAvatarsDirName)) {
        qWarning("Failed to create avatars folder");
        QString err_msg = tr("Failed to create avatars folder");
        seafApplet->errorAndExit(err_msg);
    }

    avatars_dir_ = seafile_dir.filePath(kAvatarsDirName);

    timer_->start(kCheckPendingInterval);
}

// fist check in-memory-cache, then check saved image on disk
QImage AvatarService::loadAvatarFromLocal(const QString& email)
{
    if (cache_.contains(email)) {
        return cache_.value(email);
    }

    QString path = avatarPathForEmail(seafApplet->accountManager()->currentAccount(),
                                    email);

    if (QFileInfo(path).exists()) {
        return QImage(path);
    }

    return QImage();
}

QString AvatarService::avatarPathForEmail(const Account& account, const QString& email)
{
    return QDir(avatars_dir_).filePath(::md5(account.serverUrl.host() + email));
}

void AvatarService::fetchImageFromServer(const QString& email)
{
    if (get_avatar_req_) {
        if (email == get_avatar_req_->email()) {
            return;
        }
        queue_->enqueue(email);
        return;
    }

    const Account& account = seafApplet->accountManager()->currentAccount();
    if (!account.isValid()) {
        return;
    }

    // TODO update all old avatars to newer version
    get_avatar_req_ = new GetAvatarRequest(account, email, devicePixelRatio() * 48);

    connect(get_avatar_req_, SIGNAL(success(const QImage&)),
            this, SLOT(onGetAvatarSuccess(const QImage&)));
    connect(get_avatar_req_, SIGNAL(failed(const ApiError&)),
            this, SLOT(onGetAvatarFailed(const ApiError&)));

    get_avatar_req_->send();
}

void AvatarService::onGetAvatarSuccess(const QImage& img)
{
    image_ = img;

    QString email = get_avatar_req_->email();

    cache_[email] = img;

    // save image to avatars/ folder
    QString path = avatarPathForEmail(get_avatar_req_->account(), email);
    img.save(path, "PNG");

    emit avatarUpdated(email, img);

    get_avatar_req_->deleteLater();
    get_avatar_req_ = 0;

    queue_->clearWait(email);
}

void AvatarService::onGetAvatarFailed(const ApiError& error)
{
    const QString email = get_avatar_req_->email();
    printf ("get avatar failed for %s\n", email.toUtf8().data());
    get_avatar_req_->deleteLater();
    get_avatar_req_ = 0;

    queue_->enqueueAndBackoff(email);
}

QImage AvatarService::getAvatar(const QString& email)
{
    QImage img = loadAvatarFromLocal(email);

    if (img.isNull()) {
        queue_->enqueue(email);
        return devicePixelRatio() > 1 ?
          QImage(":/images/account@2x.png") : QImage(":/images/account.png");
    } else {
        return img;
    }
}

QString AvatarService::getAvatarFilePath(const QString& email)
{
    const Account& account = seafApplet->accountManager()->currentAccount();
    return avatarPathForEmail(account, email);
}

bool AvatarService::avatarFileExists(const QString& email)
{
    return QFileInfo(getAvatarFilePath(email)).exists();
}

void AvatarService::checkPendingRequests()
{
    queue_->tick();

    if (get_avatar_req_ != 0) {
        return;
    }

    QString email = queue_->dequeue();
    if (!email.isEmpty()) {
        fetchImageFromServer(email);
    }
}

void AvatarService::onAccountChanged()
{
    queue_->reset();
    if (get_avatar_req_) {
        delete get_avatar_req_;
        get_avatar_req_ = 0;
    }
}
