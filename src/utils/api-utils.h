#ifndef SEAFILE_CLIENT_API_UTILS_H_
#define SEAFILE_CLIENT_API_UTILS_H_

QHash<QString, QString>
getSeafileLoginParams(const QString& computer_name=QString(),
                      const QString& prefix=QString());

#endif // SEAFILE_CLIENT_API_UTILS_H_
