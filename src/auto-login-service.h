#ifndef SEAFILE_CLIENT_AUTO_LOGIN_SERVICE_H_
#define SEAFILE_CLIENT_AUTO_LOGIN_SERVICE_H_

#include <QObject>
#include <QString>

#include "utils/singleton.h"

class ApiError;

class AutoLoginService : public QObject {
    SINGLETON_DEFINE(AutoLoginService)
    Q_OBJECT
public:
    AutoLoginService(QObject *parent=0);
    // Get a auto login token from server, and then open the "next_url" after login
    void startAutoLogin(const QString& next_url);

private slots:
    void onGetLoginTokenSuccess(const QString& token);
    void onGetLoginTokenFailed(const ApiError& error);
};

#endif  // SEAFILE_CLIENT_AUTO_LOGIN_SERVICE_H_
