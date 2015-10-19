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
    ServerInfoRequest* createServerInfoRequest();
public:
    ServerInfo serverInfo;
    QUrl serverUrl;
    QString username;
    QString token;
    qint64 lastVisited;
    bool isShibboleth;

    ~Account();
    Account() : serverInfoRequest(NULL), serverInfo(), lastVisited(0), isShibboleth(false) {}
    Account(QUrl serverUrl, QString username, QString token, qint64 lastVisited=0, bool isShibboleth = false)
        : serverInfoRequest(NULL),
          serverInfo(),
          serverUrl(serverUrl),
          username(username),
          token(token),
          lastVisited(lastVisited),
          isShibboleth(isShibboleth) {}

    Account(const Account &rhs)
      : serverInfoRequest(NULL),
        serverInfo(rhs.serverInfo),
        serverUrl(rhs.serverUrl),
        username(rhs.username),
        token(rhs.token),
        lastVisited(rhs.lastVisited),
        isShibboleth(rhs.isShibboleth)
    {
    }

    Account& operator=(const Account&rhs) {
        serverInfoRequest = NULL;
        serverInfo = rhs.serverInfo;
        serverUrl = rhs.serverUrl;
        username = rhs.username;
        token = rhs.token;
        lastVisited = rhs.lastVisited;
        isShibboleth = rhs.isShibboleth;
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
        return serverInfo.proEdition;
    }

    bool hasOfficePreview() const {
        return serverInfo.officePreview;
    }

    bool hasFileSearch() const {
        return serverInfo.fileSearch;
    }

    bool hasDisableSyncWithAnyFolder() const {
        return serverInfo.disableSyncWithAnyFolder;
    }

    bool isAtLeastVersion(unsigned majorVersion, unsigned minorVersion, unsigned patchVersion) const {
        return (serverInfo.majorVersion << 20) +
               (serverInfo.minorVersion << 10) +
               (serverInfo.patchVersion) >=
               (majorVersion << 20) + (minorVersion << 10) + (patchVersion);
    }

    // require pro edtions and version at least at ...
    // excluding OSS Version
    bool isAtLeastProVersion(unsigned majorVersion, unsigned minorVersion, unsigned patchVersion) const {
        return isPro() && isAtLeastVersion(majorVersion, minorVersion, patchVersion);
    }

    // require oss edtions and version at least at ...
    // excluding Pro Version
    bool isAtLeastOSSVersion(unsigned majorVersion, unsigned minorVersion, unsigned patchVersion) const {
        return !isPro() && isAtLeastVersion(majorVersion, minorVersion, patchVersion);
    }

    QUrl getAbsoluteUrl(const QString& relativeUrl) const;
    QString getSignature() const;
};

Q_DECLARE_METATYPE(Account)

#endif // ACCOUNT_H
