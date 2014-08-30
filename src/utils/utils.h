#ifndef SEAFILE_CLIENT_UTILS_H_
#define SEAFILE_CLIENT_UTILS_H_

#include <jansson.h>
#include <QString>
#include <QDir>
#include <QMap>
#include <QUrl>
#include <QSslError>
#include <QSslCipher>
#include <QSslConfiguration>
#include <QSslCertificate>

#define toCStr(_s)   ((_s).isNull() ? NULL : (_s).toUtf8().data())

#if defined(XCODE_APP)
#define RESOURCE_PATH(_name)  (QDir(QCoreApplication::applicationDirPath()).filePath(QString("../Resources/")+(_name)))
#else
#define RESOURCE_PATH(_name)  (_name)
#endif

struct sqlite3;
struct sqlite3_stmt;

typedef bool (*SqliteRowFunc) (sqlite3_stmt *stmt, void *data);

sqlite3_stmt *sqlite_query_prepare (sqlite3 *db, const char *sql);

int sqlite_query_exec (sqlite3 *db, const char *sql);

int sqlite_foreach_selected_row (sqlite3 *db, const char *sql,
                                 SqliteRowFunc callback, void *data);

int checkdir_with_mkdir (const char *dir);

int get_seafile_auto_start();

int set_seafile_auto_start(bool on);

bool get_seafile_hide_dock_icon();

int set_seafile_hide_dock_icon(bool on);

int init_seafile_hide_dock_icon();

typedef bool (*KeyValueFunc) (void *data, const char *key,
                              const char *value);

bool parse_key_value_pairs (char *string, KeyValueFunc func, void *data);

QString translateCommitTime(qint64 timestamp);

QString readableFileSize(qint64 size);

QMap<QString, QVariant> mapFromJSON(json_t *json, json_error_t *error);

QString defaultCcnetDir();

QString md5(const QString& s);

QUrl urlJoin(const QUrl& url, const QString& tail);

QString dumpHexPresentation(const QByteArray &bytes);

QString dumpSslErrors(const QList<QSslError>&);

QString dumpCipher(const QSslCipher &cipher);

QString dumpCertificate(const QSslCertificate &cert);

QString dumpCertificateFingerprint(const QSslCertificate &cert, const QCryptographicHash::Algorithm &algorithm = QCryptographicHash::Md5);

#endif
