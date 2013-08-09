#ifndef SEAFILE_API_REQUEST_H
#define SEAFILE_API_REQUEST_H


#include <QObject>

class QNetworkAccessManager;
class QNetworkRequest;
class QNetworkReply;

class SeafileApiRequest : QObject {
    Q_OBJECT
//     Q_DISABLE_COPY(SeafileApiRequest)

// public:
//     SeafileApiRequest(const Account& account, QString api_path);
//     virtual ~SeafileApiRequest();

// signals:
//     void success(const json_t* response);
//     void failed(int code, const QString& reason);

// private slots:
//     void onHttpRequestFinished();

// private:
//     Account account_;
//     QNetworkAccessManager *na_mgr_;
//     QNetworkRequest *request_;
//     QNetworkReply *reply_;
};

#endif // SEAFILE_API_REQUEST_H
