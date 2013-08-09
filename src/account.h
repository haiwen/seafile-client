#ifndef ACCOUNT_H
#define ACCOUNT_H

#include <QUrl>
#include <QString>

struct Account {
    QUrl serverUrl;
    QString username;
    QString token;
    Account() {}
    Account(QUrl serverUrl, QString username, QString token) :
        serverUrl(serverUrl),
        username(username),
        token(token) {}
};

#endif // ACCOUNT_H
