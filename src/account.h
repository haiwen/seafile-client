#ifndef ACCOUNT_H
#define ACCOUNT_H

#include <QUrl>
#include <QString>
#include <QMetaType>

struct Account {
    QUrl serverUrl;
    QString username;
    QString token;
    qint64 lastVisited;

    Account() {}
    Account(QUrl serverUrl, QString username, QString token, qint64 lastVisited=0)
        : serverUrl(serverUrl),
          username(username),
          token(token),
          lastVisited(lastVisited) {}

    bool operator==(const Account& rhs) const {
        return serverUrl == rhs.serverUrl
            && username == rhs.username
            && token == rhs.token;
    }

    bool operator!=(const Account& rhs) const {
        return !(*this == rhs);
    }

    bool isValid() const {
        return token.length() > 0;
    }
};

Q_DECLARE_METATYPE(Account)

#endif // ACCOUNT_H
