#ifndef ACCOUNT_H
#define ACCOUNT_H

#include <QUrl>
#include <QString>
#include <QMetaType>

#include "api/server-info.h"

class ServerInfoRequest;
class Account {
    friend class AccountManager;
    ServerInfoRequest *serverInfoRequest;
    ServerInfo serverInfo;
    ServerInfoRequest* createServerInfoRequest();
public:
    QUrl serverUrl;
    QString username;
    QString token;
    qint64 lastVisited;

    ~Account();
    Account() : serverInfoRequest(NULL), serverInfo() {}
    Account(QUrl serverUrl, QString username, QString token, qint64 lastVisited=0)
        : serverInfoRequest(NULL),
          serverInfo(),
          serverUrl(serverUrl),
          username(username),
          token(token),
          lastVisited(lastVisited) {}

    Account(const Account &rhs)
      : serverInfoRequest(NULL),
        serverInfo(rhs.serverInfo),
        serverUrl(rhs.serverUrl),
        username(rhs.username),
        token(rhs.token),
        lastVisited(rhs.lastVisited)
    {
    }

    Account& operator=(const Account&rhs) {
        serverInfoRequest = NULL;
        serverInfo = rhs.serverInfo;
        serverUrl = rhs.serverUrl;
        username = rhs.username;
        token = rhs.token;
        lastVisited = rhs.lastVisited;
        return *this;
    }

    bool operator==(const Account& rhs) const {
        return serverUrl == rhs.serverUrl
            && username == rhs.username;
    }

    bool operator!=(const Account& rhs) const {
        return !(*this == rhs);
    }

    bool isValid() const {
        return token.length() > 0;
    }

    bool isPro() const {
        return serverInfo.pro;
    }

    bool isAtLeastVersion(unsigned majorVersion, unsigned minorVersion, unsigned patchVersion) const {
        return serverInfo.majorVersion >= majorVersion &&
               serverInfo.minorVersion >= minorVersion &&
               serverInfo.patchVersion >= patchVersion;
    }

    unsigned feature() const {
        return serverInfo.feature;
    }

    QUrl getAbsoluteUrl(const QString& relativeUrl) const;
    QString getSignature() const;
};

Q_DECLARE_METATYPE(Account)

#endif // ACCOUNT_H
