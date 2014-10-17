#ifndef SEAFILE_CLIENT_FILE_UTILS_H_
#define SEAFILE_CLIENT_FILE_UTILS_H_

class QStringList;

QString getIconByFileName(const QString& fileName);
QString getIconByFileNameV2(const QString& fileName);

QString pathJoin(const QString& a, const QString& b);
QString pathJoin(const QString& a, const QString& b, const QString& c);
QString pathJoin(const QString& a, const QStringList& rest);

bool createDirIfNotExists(const QString& path);


#endif // SEAFILE_CLIENT_FILE_UTILS_H_
