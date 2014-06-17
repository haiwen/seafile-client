#include <QDir>
#include <QImage>

#include "seafile-applet.h"
#include "configurator.h"
#include "account-mgr.h"
#include "api/requests.h"
#include "utils/utils.h"

#include "avatar-service.h"

namespace {

const char *kAvatarsDirName = "avatars";

} // namespace

AvatarService* AvatarService::singleton_;

AvatarService* AvatarService::instance()
{
    if (singleton_ == NULL) {
        singleton_ = new AvatarService;
    }

    return singleton_;
}


AvatarService::AvatarService(QObject *parent)
    : QObject(parent)
{
    get_avatar_req_ = 0;
}

void AvatarService::start()
{
    QDir seafile_dir(seafApplet->configurator()->seafileDir());

    if (!seafile_dir.mkpath(kAvatarsDirName)) {
        qDebug("Failed to create avatars folder");
        QString err_msg = tr("Failed to create avatars folder");
        seafApplet->errorAndExit(err_msg);
    }

    avatars_dir_ = seafile_dir.filePath(kAvatarsDirName);
}

// fist check in-memory-cache, then check saved image on disk
QImage AvatarService::loadAvatarFromLocal(const QString& email)
{
    if (cache_.contains(email)) {
        return cache_.value(email);
    }

    QString path = filePathForEmail(email);

    if (QFileInfo(path).exists()) {
        return QImage(path);
    }

    return QImage();
}

QString AvatarService::filePathForEmail(const QString& email)
{
    return QDir(avatars_dir_).filePath(::md5(email));
}

void AvatarService::addEmailToDownloadQueue(const QString& email)
{
    if (!pending_emails_.contains(email)) {
        pending_emails_.enqueue(email);
    }
}

void AvatarService::fetchImageFromServer(const QString& email)
{
    if (get_avatar_req_) {
        if (email == get_avatar_req_->email()) {
            return;
        }
        addEmailToDownloadQueue(email);
        return;
    }

    const Account& account = seafApplet->accountManager()->currentAccount();
    if (!account.isValid()) {
        return;
    }

    get_avatar_req_ = new GetAvatarRequest(account, email, 36);

    connect(get_avatar_req_, SIGNAL(success(const QImage&)),
            this, SLOT(onGetAvatarSuccess(const QImage&)));
    connect(get_avatar_req_, SIGNAL(failed(const ApiError&)),
            this, SLOT(onGetAvatarFailed(const ApiError&)));

    get_avatar_req_->send();
}

void AvatarService::onGetAvatarSuccess(const QImage& img)
{
    printf ("get avatar success, size = (%d, %d)\n", img.width(), img.height());
    image_ = img;

    QString email = get_avatar_req_->email();

    emit avatarUpdated(email, img);

    cache_[email] = img;

    QString path = filePathForEmail(email);

    // save image to avatars/ folder
    img.save(path, "PNG");

    get_avatar_req_->deleteLater();
    get_avatar_req_ = 0;

    if (!pending_emails_.isEmpty()) {
        fetchImageFromServer(pending_emails_.dequeue());
    }
}

void AvatarService::onGetAvatarFailed(const ApiError& error)
{
    printf ("get avatar failed\n");
    get_avatar_req_->deleteLater();
    get_avatar_req_ = 0;

    if (!pending_emails_.isEmpty()) {
        fetchImageFromServer(pending_emails_.dequeue());
    }
}

QImage AvatarService::getAvatar(const QString& email)
{
    QImage img = loadAvatarFromLocal(email);

    if (img.isNull()) {
        fetchImageFromServer(email);
        return QImage(":/images/account-36.png");
    } else {
        return img;
    }
}
