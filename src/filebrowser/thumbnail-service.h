#ifndef SEAFILE_CLIENT_THUMBNAIL_SERVICE_H
#define SEAFILE_CLIENT_THUMBNAIL_SERVICE_H

#include <vector>
#include <QObject>
#include <QImage>
#include <QPixmapCache>
#include <QString>
#include <QPixmap>

class QTimer;

class Account;
class ApiError;
class GetThumbnailRequest;
class PendingThumbnailRequestQueue;


class ThumbnailService : public QObject
{
    Q_OBJECT
public:
    static ThumbnailService* instance();

    void start();

    QPixmap getThumbnail(const QString& repo_id, const QString& path, uint thumbnail_default_size = 28);

signals:
    void thumbnailUpdated(const QPixmap& thumbnail, const QString& path);

private slots:
    void onGetThumbnailSuccess(const QPixmap& thumbnail);
    void onGetThumbnailFailed(const ApiError& error);
    void checkPendingRequests();

private:
    Q_DISABLE_COPY(ThumbnailService)

    ThumbnailService(QObject *parent=0);

    static ThumbnailService *singleton_;

    QPixmap loadThumbnailFromLocal(const QString& repo_id, const QString& path, uint size);
    void fetchImageFromServer(const QString& repo_id, const QString& path, uint size);
    QString getPixmapCacheKey(const QString& repo_id, const QString& path, uint size);

    GetThumbnailRequest *get_thumbnail_req_;

    QString thumbnails_dir_;

    QPixmapCache cache_;

    PendingThumbnailRequestQueue *queue_;

    QTimer *timer_;
    
};

#endif // SEAFILE_CLIENT_AVATAR_SERVICE_H
