#ifndef SEAFILE_CLIENT_AVATAR_SERVICE_H
#define SEAFILE_CLIENT_AVATAR_SERVICE_H

#include <vector>
#include <QObject>
#include <QImage>
#include <QHash>
#include <QString>

class QImage;
class QTimer;

class Account;
class ApiError;
class GetAvatarRequest;
class PendingAvatarRequestQueue;

struct sqlite3;

class AvatarService : public QObject
{
    Q_OBJECT
public:
    static AvatarService* instance();

    void start();

    QImage getAvatar(const QString& email);

    static const int kAvatarSize;

signals:
    void avatarUpdated(const QString& email, const QImage& avatar);

private slots:
    void onGetAvatarSuccess(const QImage& img);
    void onGetAvatarFailed(const ApiError& error);
    void checkPendingRequests();
    void onAccountChanged();

private:
    Q_DISABLE_COPY(AvatarService)

    AvatarService(QObject *parent=0);

    static AvatarService *singleton_;

    QImage loadAvatarFromLocal(const QString& email);
    void fetchImageFromServer(const QString& email);
    QString avatarPathForEmail(const Account& account, const QString& email);
    QString getAvatarFilePath(const QString& email);
    bool avatarFileExists(const QString& email);

    GetAvatarRequest *get_avatar_req_;

    QString avatars_dir_;

    QImage image_;

    QHash<QString, QImage> cache_;

    PendingAvatarRequestQueue *queue_;

    QTimer *timer_;

    struct sqlite3 *autoupdate_db_;
};


#endif // SEAFILE_CLIENT_AVATAR_SERVICE_H
