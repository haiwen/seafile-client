#ifndef SEAFILE_CLIENT_FILE_UTILS_H_
#define SEAFILE_CLIENT_FILE_UTILS_H_

class QStringList;

QString getIconByFileName(const QString& fileName);
QString getIconByFileNameV2(const QString& fileName);

QString mimeTypeFromFileName(const QString& fileName);

QString getParentPath(const QString& path);
QString getBaseName(const QString& path);

QString pathJoin(const QString& a, const QString& b);
QString pathJoin(const QString& a, const QString& b, const QString& c);
QString pathJoin(const QString& a, const QString& b, const QString& c, const QString& d);
QString pathJoin(const QString& a, const QStringList& rest);

bool createDirIfNotExists(const QString& path);


#endif // SEAFILE_CLIENT_FILE_UTILS_H_
