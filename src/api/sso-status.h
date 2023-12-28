#ifndef SEAFILE_CLIENT_API_SSO_STATUS_H
#define SEAFILE_CLIENT_API_SSO_STATUS_H

#include <QString>

class ClientSSOStatus
{
public:
    QString status;
    QString username;
    QString api_token;
};

#endif // SEAFILE_CLIENT_API_SSO_STATUS_H
