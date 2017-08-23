#include <QDir>
#include <QImage>
#include <QQueue>
#include <QHash>
#include <QTimer>
#include <QDateTime>

#include "seafile-applet.h"
#include "configurator.h"
#include "account-mgr.h"
#include "api/requests.h"
#include "utils/paint-utils.h"
#include "utils/utils.h"

#include <sqlite3.h>
#include "avatar-service.h"

namespace {

const int kCheckPendingInterval = 1000; // 1s
const char *kAvatarsDirName = "avatars";
const qint64 kExpireTimeIntevalMsec = 300 * 1000; // 5min

bool loadTimeStampCB(sqlite3_stmt *stmt, void* data)
{
    qint64* mtime = reinterpret_cast<qint64*>(data);

    *mtime = sqlite3_column_int64(stmt, 0);

    return true;
}

} // namespace

const int AvatarService::kAvatarSize = 40;
const int kAvatarSizeFromServer = 80;

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

class PendingAvatarRequestQueue
{
public:
    PendingAvatarRequestQueue() {};

    void enqueue(const QString& email) {
        if (q_.contains(email)) {
            return;
        }
        // if we have set an expire time, and we haven't reached it yet
        if (expire_time_.contains(email) &&
            QDateTime::currentMSecsSinceEpoch() <= expire_time_[email]) {
            return;
        }
        // update expire time
        expire_time_[email] = QDateTime::currentMSecsSinceEpoch() + kExpireTimeIntevalMsec;

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
        expire_time_.clear();
    }

private:
    QQueue<QString> q_;

    QHash<QString, PendingRequestInfo> wait_;
    QHash<QString, qint64> expire_time_;
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
    : QObject(parent), get_avatar_req_(NULL)
{
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

    do {
        const char *errmsg;
        QString db_path = QDir(seafApplet->configurator()->seafileDir()).filePath("accounts.db");
        if (sqlite3_open (db_path.toUtf8().data(), &autoupdate_db_)) {
            errmsg = sqlite3_errmsg (autoupdate_db_);
            qWarning("failed to avatar autoupdate database %s: %s",
                    db_path.toUtf8().data(), errmsg ? errmsg : "no error given");

            sqlite3_close(autoupdate_db_);
            autoupdate_db_ = NULL;
            break;
        }

        // enabling foreign keys, it must be done manually from each connection
        // and this feature is only supported from sqlite 3.6.19
        const char *sql = "PRAGMA foreign_keys=ON;";
        if (sqlite_query_exec (autoupdate_db_, sql) < 0) {
            qWarning("sqlite version is too low to support foreign key feature\n");
            qWarning("feature avatar autoupdate is disabled\n");
            sqlite3_close(autoupdate_db_);
            autoupdate_db_ = NULL;
            break;
        }

        // create Avatar table
        sql = "CREATE TABLE IF NOT EXISTS Avatar ("
            "filename TEXT PRIMARY KEY, timestamp BIGINT, "
            "url VARCHAR(24), username VARCHAR(15), "
            "FOREIGN KEY(url, username) REFERENCES Accounts(url, username) "
            "ON DELETE CASCADE ON UPDATE CASCADE )";
        if (sqlite_query_exec (autoupdate_db_, sql) < 0) {
            qWarning("failed to create avatar table\n");
            sqlite3_close(autoupdate_db_);
            autoupdate_db_ = NULL;
        }
    } while (0);

    timer_->start(kCheckPendingInterval);
}

// fist check in-memory-cache, then check saved image on disk
QImage AvatarService::loadAvatarFromLocal(const QString& email)
{
    if (cache_.contains(email)) {
        return cache_.value(email);
    }

    QImage ret;
    if (avatarFileExists(email)) {
        ret = QImage(getAvatarFilePath(email));
        cache_[email] = ret;
    }

    return ret;
}

QString AvatarService::avatarPathForEmail(const Account& account, const QString& email)
{
    return QDir(avatars_dir_)
        .filePath(::md5(account.serverUrl.host() + email + "/" +
                        QString::number(kAvatarSizeFromServer)));
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

    if (!seafApplet->accountManager()->hasAccount())
        return;
    const Account& account = seafApplet->accountManager()->accounts().front();
    qint64 mtime = 0;

    if (autoupdate_db_) {
        char *zql = sqlite3_mprintf("SELECT timestamp FROM Avatar "
                                    "WHERE filename = %Q",
                                    avatarPathForEmail(account, email).toUtf8().data());
        sqlite_foreach_selected_row(autoupdate_db_, zql, loadTimeStampCB, &mtime);
        sqlite3_free(zql);
    }

    get_avatar_req_ = new GetAvatarRequest(account, email, mtime, kAvatarSizeFromServer);

    connect(get_avatar_req_, SIGNAL(success(const QImage&)),
            this, SLOT(onGetAvatarSuccess(const QImage&)));
    connect(get_avatar_req_, SIGNAL(failed(const ApiError&)),
            this, SLOT(onGetAvatarFailed(const ApiError&)));

    get_avatar_req_->send();
}

void AvatarService::onGetAvatarSuccess(const QImage& img)
{
    if (!get_avatar_req_) {
        return;
    }

    const QString email = get_avatar_req_->email();

    // if no change? early return
    if (img.isNull()) {
        get_avatar_req_->deleteLater();
        get_avatar_req_ = NULL;

        queue_->clearWait(email);
        return;
    }

    image_ = img;

    cache_[email] = img;

    // save image to avatars/ folder
    QString path = avatarPathForEmail(get_avatar_req_->account(), email);
    if (!img.save(path, "PNG")) {
        qWarning("Unable to save new avatar file %s", path.toUtf8().data());
    }

    // update cache db
    if (autoupdate_db_) {
        QString mtime = QString::number(get_avatar_req_->mtime());
        char *zql = sqlite3_mprintf(
            "REPLACE INTO Avatar(filename, timestamp, url, username) "
            "VALUES (%Q, %Q, %Q, %Q)",
            path.toUtf8().data(),
            mtime.toUtf8().data(),
            get_avatar_req_->account().serverUrl.toEncoded().data(),
            get_avatar_req_->account().username.toUtf8().data());
        sqlite_query_exec(autoupdate_db_, zql);
        sqlite3_free(zql);
    }

    emit avatarUpdated(email, img);

    get_avatar_req_->deleteLater();
    get_avatar_req_ = NULL;

    queue_->clearWait(email);
}

void AvatarService::onGetAvatarFailed(const ApiError& error)
{
    if (!get_avatar_req_) {
        return;
    }
    const QString email = get_avatar_req_->email();
    get_avatar_req_->deleteLater();
    get_avatar_req_ = NULL;

    queue_->enqueueAndBackoff(email);
}

QImage AvatarService::getAvatar(const QString& email)
{
    QImage img = loadAvatarFromLocal(email);

    // TODO: check the timestamp of the cached avatar and update it if too old,
    // e.g. cached more than one hour ago. We use timestamps when asking the
    // server for avatars so updating avatars should be a light weight
    // operation.

    // update all avatars if feature autoupdate enabled or img is null
    if (autoupdate_db_ || img.isNull()) {
        if (!get_avatar_req_ || get_avatar_req_->email() != email) {
            queue_->enqueue(email);
        }
    }
    if (img.isNull()) {
        return QImage(":/images/account.png");
    } else {
        return img;
    }
}

QString AvatarService::getAvatarFilePath(const QString& email)
{
    const Account& account = seafApplet->accountManager()->accounts().front();
    return avatarPathForEmail(account, email);
}

bool AvatarService::avatarFileExists(const QString& email)
{
    QString path = getAvatarFilePath(email);
    bool ret = QFileInfo(path).exists();

    if (!ret) {
        char *zql = sqlite3_mprintf("DELETE FROM Avatar WHERE filename = %Q", path.toUtf8().data());
        sqlite_query_exec (autoupdate_db_, zql);
        sqlite3_free(zql);
    }
    return ret;
}

void AvatarService::checkPendingRequests()
{
    queue_->tick();

    if (get_avatar_req_ != NULL) {
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
        get_avatar_req_->deleteLater();
        get_avatar_req_ = NULL;
    }
    cache_.clear();
}
