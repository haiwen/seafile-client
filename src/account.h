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

    bool operator==(const Account& rhs) const {
        return serverUrl == rhs.serverUrl && username == rhs.username && token == rhs.token;
    }

    bool operator!=(const Account& rhs) const {
        return !(*this == rhs);
    }
};

#endif // ACCOUNT_H
