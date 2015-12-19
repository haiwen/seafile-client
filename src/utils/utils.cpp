#include <cassert>
#include <errno.h>
#include <dirent.h>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <sqlite3.h>
#include <glib.h>
#include <cstring>
#include <QObject>
#include <QString>
#include <QSettings>
#include <QProcess>
#include <QDesktopServices>
#include <QHostInfo>
#include <jansson.h>
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
#include <QUrlQuery>
#endif

#include "utils/utils-mac.h"
#include "utils/utils-win.h"

#if defined(Q_OS_MAC)
    #include <sys/sysctl.h>
#elif defined(Q_OS_WIN32)
    #include <windows.h>
    #include <psapi.h>
#endif

#include <QMap>
#include <QVariant>
#include <QDebug>
#include <QDateTime>
#include <QCryptographicHash>
#include <QSslCipher>
#include <QSslCertificate>

#include "seafile-applet.h"
#include "rpc/rpc-client.h"

#include "utils/utils.h"

namespace {

const char *kSeafileClientBrand = "Horizonbase";
#if defined(Q_OS_WIN32)
const char *kCcnetConfDir = "ccnet";
#else
const char *kCcnetConfDir = ".ccnet";
#endif

#ifdef Q_OS_LINUX
/// \brief call xdg-mime to find out the mime filetype X11 recognizes it as
/// xdg-mime's usage:
/// xdg-mime query filetype <filename>
/// stdout: mime-type
bool getMimeTypeFromXdgUtils(const QString &filepath, QString *mime)
{
    QProcess subprocess;
    QStringList args("query");
    args.push_back("filetype");
    args.push_back(filepath);
    subprocess.start(QLatin1String("xdg-mime"), args);
    subprocess.waitForFinished(-1);
    if (subprocess.exitCode())
        return false;
    *mime = subprocess.readAllStandardOutput();
    *mime = mime->trimmed();
    if (mime->isEmpty())
        return false;
    return true;
}

/// \brief call xdg-mime to find out the application X11 opens with by mime filetype
/// xdg-mime's usage:
/// xdg-mime query default <filename>
/// stdout: application
bool getOpenApplicationFromXdgUtils(const QString &mime, QString *application)
{
    QProcess subprocess;
    QStringList args("query");
    args.push_back("default");
    args.push_back(mime);
    subprocess.start(QLatin1String("xdg-mime"), args);
    subprocess.waitForFinished(-1);
    if (subprocess.exitCode())
        return false;
    *application = subprocess.readAllStandardOutput();
    *application = application->trimmed();
    if (application->isEmpty())
        return false;
    return true;
}
#endif

} // namespace


QString defaultCcnetDir() {
    const char *env = g_getenv("CCNET_CONF_DIR");
    if (env) {
        return QString::fromUtf8(env);
    } else {
        return QDir::home().filePath(kCcnetConfDir);
    }
}

QString defaultDownloadDir() {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    static QStringList list = QStandardPaths::standardLocations(QStandardPaths::DownloadLocation);
    if (!list.empty())
        return list.front();
#endif
    // qt4 don't have QStandardPaths, use glib's as fallback
    return QString::fromUtf8(g_get_user_special_dir(G_USER_DIRECTORY_DOWNLOAD));
}

bool openInNativeExtension(const QString &path) {
#if defined(Q_OS_WIN32)
    //call ShellExecute internally
    return QDesktopServices::openUrl(QUrl::fromLocalFile(path));
#elif defined(Q_OS_MAC)
    // mac's open program, it will fork to open the file in a subprocess
    // so we will wait for it to check whether it succeeds or not
    QProcess subprocess;
    subprocess.start(QLatin1String("open"), QStringList(path));
    subprocess.waitForFinished(-1);
    return subprocess.exitCode() == 0;
#elif defined(Q_OS_LINUX)
    // unlike mac's open program, xdg-open won't fork a new subprocess to open
    // the file will block until the application returns, so we won't wait for it
    // and we need another approach to check if it works

    // find out if the file can be opened by xdg-open, xdg-mime
    // usually they are installed in xdg-utils installed by default
    QString mime_type;
    if (!getMimeTypeFromXdgUtils(path, &mime_type))
        return false;
    // don't open this type of files from xdg-mime
    if (mime_type == "application/octet-stream")
        return false;
    // in fact we need to filter out files like application/x-executable
    // but it is not necessary since getMimeTypeFromXdg will return false for
    // it!
    QString application;
    if (!getOpenApplicationFromXdgUtils(mime_type, &application))
        return false;

    return QProcess::startDetached(QLatin1String("xdg-open"),
                                   QStringList(path));
#else
    return false;
#endif
}

bool showInGraphicalShell(const QString& path) {
#if defined(Q_OS_WIN32)
    QStringList params;
    if (!QFileInfo(path).isDir())
        params << QLatin1String("/select,");
    params << QDir::toNativeSeparators(path);
    return QProcess::startDetached(QLatin1String("explorer.exe"), params);
#elif defined(Q_OS_MAC)
    QStringList scriptArgs;
    scriptArgs << QLatin1String("-e")
               << QString::fromLatin1("tell application \"Finder\" to reveal POSIX file \"%1\"")
                                     .arg(path);
    QProcess::execute(QLatin1String("/usr/bin/osascript"), scriptArgs);
    scriptArgs.clear();
    scriptArgs << QLatin1String("-e")
               << QLatin1String("tell application \"Finder\" to activate");
    QProcess::execute("/usr/bin/osascript", scriptArgs);
    return true;
#else
    return QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(path).absolutePath()));
#endif
}

typedef bool (*SqliteRowFunc) (sqlite3_stmt *stmt, void *data);

sqlite3_stmt *
sqlite_query_prepare (sqlite3 *db, const char *sql)
{
    sqlite3_stmt *stmt;
    int result;

    result = sqlite3_prepare_v2 (db, sql, -1, &stmt, NULL);

    if (result != SQLITE_OK) {
        const gchar *str = sqlite3_errmsg (db);

        g_warning ("Couldn't prepare query, error:%d->'%s'\n\t%s\n",
                   result, str ? str : "no error given", sql);

        return NULL;
    }

    return stmt;
}

int sqlite_query_exec (sqlite3 *db, const char *sql)
{
    char *errmsg = NULL;
    int result;

    result = sqlite3_exec (db, sql, NULL, NULL, &errmsg);

    if (result != SQLITE_OK) {
        if (errmsg != NULL) {
            g_warning ("SQL error: %d - %s\n:\t%s\n", result, errmsg, sql);
            sqlite3_free (errmsg);
        }
        return -1;
    }

    return 0;
}

int sqlite_foreach_selected_row (sqlite3 *db, const char *sql,
                                 SqliteRowFunc callback, void *data)
{
    sqlite3_stmt *stmt;
    int result;
    int n_rows = 0;

    stmt = sqlite_query_prepare (db, sql);
    if (!stmt) {
        return -1;
    }

    while (1) {
        result = sqlite3_step (stmt);
        if (result != SQLITE_ROW)
            break;
        n_rows++;
        if (!callback (stmt, data))
            break;
    }

    if (result == SQLITE_ERROR) {
        const gchar *s = sqlite3_errmsg (db);

        g_warning ("Couldn't execute query, error: %d->'%s'\n",
                   result, s ? s : "no error given");
        sqlite3_finalize (stmt);
        return -1;
    }

    sqlite3_finalize (stmt);
    return n_rows;
}

int checkdir_with_mkdir (const char *dir)
{
#if defined(Q_OS_WIN32)
    int ret;
    char *path = g_strdup(dir);
    char *p = (char *)path + strlen(path) - 1;
    while (*p == '\\' || *p == '/') *p-- = '\0';
    ret = g_mkdir_with_parents(path, 0755);
    g_free (path);
    return ret;
#else
    return g_mkdir_with_parents(dir, 0755);
#endif
}


#if defined(Q_OS_WIN32)
static LONG
get_win_run_key (HKEY *pKey)
{
    const char *key_run = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run";
    LONG result = RegOpenKeyEx(
        /* We don't use HKEY_LOCAL_MACHINE here because that requires
         * seaf-daemon to run with admin privilege. */
                               HKEY_CURRENT_USER,
                               key_run,
                               0L,KEY_WRITE | KEY_READ,
                               pKey);
    if (result != ERROR_SUCCESS) {
        qWarning("Failed to open Registry key %s\n", key_run);
    }

    return result;
}

static int
add_to_auto_start (const wchar_t *appname_w, const wchar_t *path_w)
{
    HKEY hKey;
    LONG result = get_win_run_key(&hKey);
    if (result != ERROR_SUCCESS) {
        return -1;
    }

    DWORD n = sizeof(wchar_t) * (wcslen(path_w) + 1);

    result = RegSetValueExW (hKey, appname_w,
                             0, REG_SZ, (const BYTE *)path_w, n);

    RegCloseKey(hKey);
    if (result != ERROR_SUCCESS) {
        qWarning("Failed to create auto start value\n");
        return -1;
    }

    return 0;
}

static int
delete_from_auto_start(const wchar_t *appname)
{
    HKEY hKey;
    LONG result = get_win_run_key(&hKey);
    if (result != ERROR_SUCCESS) {
        return -1;
    }

    result = RegDeleteValueW (hKey, appname);
    RegCloseKey(hKey);
    if (result != ERROR_SUCCESS) {
        qWarning("Failed to remove auto start value");
        return -1;
    }

    return 0;
}

int
get_seafile_auto_start()
{
    HKEY hKey;
    LONG result = get_win_run_key(&hKey);
    if (result != ERROR_SUCCESS) {
        return -1;
    }

    char buf[MAX_PATH] = {0};
    DWORD len = sizeof(buf);
    result = RegQueryValueExW (hKey,             /* Key */
                               getBrand().toStdWString().c_str(),        /* value */
                               NULL,             /* reserved */
                               NULL,             /* output type */
                               (LPBYTE)buf,      /* output data */
                               &len);            /* output length */

    RegCloseKey(hKey);
    if (result != ERROR_SUCCESS) {
        /* seafile applet auto start no set  */
        return 0;
    }

    return 1;
}

int
set_seafile_auto_start(bool on)
{
    int result = 0;
    if (on) {
        /* turn on auto start  */
        wchar_t applet_path[MAX_PATH];
        if (GetModuleFileNameW (NULL, applet_path, MAX_PATH) == 0) {
            return -1;
        }

        result = add_to_auto_start (getBrand().toStdWString().c_str(), applet_path);

    } else {
        /* turn off auto start */
        result = delete_from_auto_start(getBrand().toStdWString().c_str());
    }
    return result;
}

#elif defined(Q_OS_MAC)
int
get_seafile_auto_start()
{
    return utils::mac::get_auto_start();
}

int
set_seafile_auto_start(bool on)
{
    bool was_on = utils::mac::get_auto_start();
    if (on != was_on)
        utils::mac::set_auto_start(on);
    return on;
}
#else
int
get_seafile_auto_start()
{
    return 0;
}

int
set_seafile_auto_start(bool /* on */)
{
    return 0;
}

#endif

int
set_seafile_dock_icon_style(bool hidden)
{
#if defined(Q_OS_MAC)
    utils::mac::setDockIconStyle(hidden);
#endif
    return 0;
}

bool parse_key_value_pairs (char *string, KeyValueFunc func, void *data)
{
    char *line = string, *next, *space;
    char *key, *value;

    while (*line) {
        /* handle empty line */
        if (*line == '\n') {
            ++line;
            continue;
        }

        for (next = line; *next != '\n' && *next; ++next) ;
        *next = '\0';

        for (space = line; space < next && *space != ' '; ++space) ;
        if (*space != ' ') {
            return false;
        }
        *space = '\0';
        key = line;
        value = space + 1;

        if (func(data, key, value) == FALSE)
            return false;

        line = next + 1;
    }
    return true;
}

QString getBrand()
{
    return QString::fromUtf8(kSeafileClientBrand);
}

static
QList<QVariant> listFromJSON(json_t *array)
{
    QList<QVariant> ret;
    size_t array_size = json_array_size(array);
    json_t *value;

    for(size_t index = 0; index < array_size &&
        (value = json_array_get(array, index)); ++index) {
        /* block of code that uses index and value */
        QVariant v;
        if (json_is_object(value)) {
            v = mapFromJSON(value, NULL);
        } else if (json_is_array(value)) {
            v = listFromJSON(value);
        } else if (json_is_string(value)) {
            v = QString::fromUtf8(json_string_value(value));
        } else if (json_is_integer(value)) {
            v = json_integer_value(value);
        } else if (json_is_real(value)) {
            v = json_real_value(value);
        } else if (json_is_boolean(value)) {
            v = json_is_true(value);
        }
        if (v.isValid()) {
          ret.push_back(v);
        }
    }
    return ret;
}

QMap<QString, QVariant> mapFromJSON(json_t *json, json_error_t *error)
{
    QMap<QString, QVariant> dict;
    void *member;
    const char *key;
    json_t *value;

    for (member = json_object_iter(json); member; member = json_object_iter_next(json, member)) {
        key = json_object_iter_key(member);
        value = json_object_iter_value(member);

        QString k = QString::fromUtf8(key);
        QVariant v;

        // json_is_object(const json_t *json)
        // json_is_array(const json_t *json)
        // json_is_string(const json_t *json)
        // json_is_integer(const json_t *json)
        // json_is_real(const json_t *json)
        // json_is_true(const json_t *json)
        // json_is_false(const json_t *json)
        // json_is_null(const json_t *json)
        if (json_is_object(value)) {
            v = mapFromJSON(json, NULL);
        } else if (json_is_array(value)) {
            v = listFromJSON(value);
        } else if (json_is_string(value)) {
            v = QString::fromUtf8(json_string_value(value));
        } else if (json_is_integer(value)) {
            v = json_integer_value(value);
        } else if (json_is_real(value)) {
            v = json_real_value(value);
        } else if (json_is_boolean(value)) {
            v = json_is_true(value);
        }

        if (v.isValid()) {
            dict[k] = v;
        }
    }
    return dict;
}

QString mapToJson(QMap<QString, QVariant> map)
{
    json_t *object = NULL;
    char *info = NULL;
    object = json_object();

    Q_FOREACH (const QString &k, map.keys()) {
        QVariant v = map.value(k);
        switch (v.type()) {
        case QVariant::String:
            json_object_set_new(object, toCStr(k), json_string(toCStr(v.toString())));
            break;
        case QVariant::Int:
            json_object_set_new(object, toCStr(k), json_integer(v.toInt()));
            break;
            // TODO: support other types
        default:
            continue;
        }
    }

    info = json_dumps(object, 0);
    QString ret = QString::fromUtf8(info);
    json_decref (object);
    free (info);
    return ret;
}

QString translateCommitTime(qint64 timestamp) {
    timestamp *= 1000;          // use milli seconds
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (now <= timestamp) {
        return QObject::tr("Just now");
    }

    qint64 delta = (now - timestamp) / 1000;

    qint64 secondsPerDay = 24 * 60 * 60;

    qint64 days = delta / secondsPerDay;
    qint64 seconds = delta % secondsPerDay;

    QDateTime dt = QDateTime::fromMSecsSinceEpoch(timestamp);

    if (days >= 14) {
        return dt.toString("yyyy-MM-dd");

    } else if (days > 0) {
        return days == 1 ? QObject::tr("1 day ago") : QObject::tr("%1 days ago").arg(days);

    } else if (seconds >= 60 * 60) {
        qint64 hours = seconds / 3600;
        return hours == 1 ? QObject::tr("1 hour ago") : QObject::tr("%1 hours ago").arg(hours);

    } else if (seconds >= 60) {
        qint64 minutes = seconds / 60;
        return minutes == 1 ? QObject::tr("1 minute ago") : QObject::tr("%1 minutes ago").arg(minutes);

    } else if (seconds > 0) {
        // return seconds == 1 ? QObject::tr("1 second ago") : QObject::tr("%1 seconds ago").arg(seconds);
        return QObject::tr("Just now");

    } else {
        return QObject::tr("Just now");
    }
}

QString readableFileSize(qint64 size)
{
    QString str;

    if (size <= 1024) {
        str = "B";
    } else if (size > 1024 && size <= 1024*1024) {
        size = size / 1024;
        str = "KB";
    } else if (size > 1024*1024 && size <= 1024*1024*1024) {
        size = size / 1024 / 1024;
        str = "MB";
    } else if (size > 1024*1024*1024) {
        size = size / 1024 / 1024 / 1024;
        str = "GB";
    }

    return QString::number(size) + str;
}

QString readableFileSizeV2(qint64 size)
{
    if (size <= 0)
        return "0 B";
    //max size is 1 x 7 + 1
    const int bufsize = 10;
    char buf[bufsize];

    int size_unit_b = size & 1023; //B
    int size_unit_k = size >> 10 & 1023; //K
    int size_unit_m = size >> 20 & 1023; //M
    int size_unit_g = size >> 30 & 1023; //G
    int size_unit_t = size >> 40 & 1023; //T
    int size_unit_p = size >> 50 & 1023; //P
    //omit all size large than 1PB

    if (size_unit_p)
        snprintf(buf, bufsize - 1,
                 "%d.%.2dP",
                 size_unit_p, (size_unit_t * 100) >> 10);
    else if (size_unit_t)
        snprintf(buf, bufsize - 1,
                 "%d.%.2dT",
                 size_unit_t, (size_unit_g * 100) >> 10);
    else if (size_unit_g)
        snprintf(buf, bufsize - 1,
                 "%d.%.2dG",
                 size_unit_g, (size_unit_m * 100) >> 10);
    else if (size_unit_m)
        snprintf(buf, bufsize - 1,
                 "%d.%.2dM",
                 size_unit_m, (size_unit_k * 100) >> 10);
    else if (size_unit_k)
        snprintf(buf, bufsize - 1,
                 "%d.%.2dK",
                 size_unit_k, (size_unit_b * 100) >> 10);
    else
        snprintf(buf, bufsize - 1,
                 "%dB",
                 size_unit_b);

    return buf; // return by a new QString item
}

QString md5(const QString& s)
{
    return QCryptographicHash::hash(s.toUtf8(), QCryptographicHash::Md5).toHex();
}

QUrl urlJoin(const QUrl& head, const QString& tail)
{
    QString a = head.toString();
    QString b = tail;

    if (!a.endsWith("/")) {
        a += "/";
    }
    while (b.startsWith("/")) {
        b = b.mid(1);
    }
    return QUrl(a + b);
}

void removeDirRecursively(const QString &path)
{
    QFileInfo file_info(path);
    if (file_info.isDir()) {
        QDir dir(path);
        QStringList file_list = dir.entryList();
        for (int i = 0; i < file_list.count(); ++i) {
            removeDirRecursively(file_list.at(i));
        }
        removeDirRecursively(path);
    } else {
        QFile::remove(path);
    }
}

QString dumpHexPresentation(const QByteArray &bytes)
{
    if (bytes.size() < 2)
      return QString(bytes).toUpper();
    QString output((char)bytes[0]);
    output += (char)bytes[1];
    for (int i = 2 ; i != bytes.size() ; i++) {
      if (i % 2 == 0)
        output += ':';
      output += (char)bytes[i];
    }
    return output.toUpper();
}

QString dumpCipher(const QSslCipher &cipher)
{
    QString s = "\n";
    s += "Authentication:  " + cipher.authenticationMethod() + "\n";
    s += "Encryption:      " + cipher.encryptionMethod() + "\n";
    s += "Key Exchange:    " + cipher.keyExchangeMethod() + "\n";
    s += "Cipher Name:     " + cipher.name() + "\n";
    s += "Protocol:        " +  cipher.protocolString() + "\n";
    s += "Supported Bits:  " + QString(cipher.supportedBits()) + "\n";
    s += "Used Bits:       " + QString(cipher.usedBits()) + "\n";
    return s;
}

QString dumpCertificate(const QSslCertificate &cert)
{
    if (cert.isNull())
      return "\n-\n";

    QString s = "\n";
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    s += cert.toText();
#else
    QString s_none = QObject::tr("<Not Part of Certificate>");
    #define CERTIFICATE_STR(x) ( ((x) == "" ) ? s_none : (x) )

    s += "Certificate:\n";
    s += "\nIssued To:\n";
    s += "CommonName(CN):             " + CERTIFICATE_STR(cert.subjectInfo(QSslCertificate::CommonName)) + "\n";
    s += "Organization(O):            " + CERTIFICATE_STR(cert.subjectInfo(QSslCertificate::Organization)) + "\n";
    s += "OrganizationalUnitName(OU): " + CERTIFICATE_STR(cert.subjectInfo(QSslCertificate::OrganizationalUnitName)) + "\n";
    s += "Serial Number:              " + dumpHexPresentation(cert.serialNumber()) + "\n";

    s += "\nIssued By:\n";
    s += "CommonName(CN):             " + CERTIFICATE_STR(cert.issuerInfo(QSslCertificate::CommonName)) + "\n";
    s += "Organization(O):            " + CERTIFICATE_STR(cert.issuerInfo(QSslCertificate::Organization)) + "\n";
    s += "OrganizationalUnitName(OU): " + CERTIFICATE_STR(cert.issuerInfo(QSslCertificate::OrganizationalUnitName)) + "\n";

    s += "\nPeriod Of Validity\n";
    s += "Begins On:    " + cert.effectiveDate().toString() + "\n";
    s += "Expires On:   " + cert.expiryDate().toString() + "\n";
    s += "IsValid:      " + (cert.isValid() ? QString("Yes") : QString("No")) + "\n";

    s += "\nFingerprints\n";
    s += "SHA1 Fingerprint:\n" + dumpCertificateFingerprint(cert, QCryptographicHash::Sha1) + "\n";
    s += "MD5 Fingerprint:\n" + dumpCertificateFingerprint(cert, QCryptographicHash::Md5) + "\n";
#endif

    s += "\n\n";
    s += cert.toPem();

    return s;
}

QString dumpCertificateFingerprint(const QSslCertificate &cert, const QCryptographicHash::Algorithm &algorithm)
{
    if(cert.isNull())
      return "";
    return dumpHexPresentation(cert.digest(algorithm).toHex());
}

QString dumpSslErrors(const QList<QSslError> &errors)
{
    QString s;
    foreach (const QSslError &error, errors) {
        s += error.errorString() + "\n";
    }
    return s;
}

void msleep(int mseconds)
{
#ifdef Q_OS_WIN32
    ::Sleep(mseconds);
#else
    struct timespec ts;
    ts.tv_sec = mseconds / 1000;
    ts.tv_nsec = mseconds % 1000 * 1000 * 1000;

    int r;
    do {
        r = ::nanosleep(&ts, &ts);
    } while (r == -1 && errno == EINTR);
#endif
}

QUrl includeQueryParams(const QUrl& url,
                        const QHash<QString, QString>& params)
{
    QUrl u(url);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    QUrlQuery query;
    Q_FOREACH (const QString& key, params.keys()) {
        QString value = params[key];
        query.addQueryItem(QUrl::toPercentEncoding(key),
                           QUrl::toPercentEncoding(value));
    }
    u.setQuery(query);
#else
    Q_FOREACH (const QString& key, params.keys()) {
        QString value = params[key];
        u.addEncodedQueryItem(QUrl::toPercentEncoding(key),
                              QUrl::toPercentEncoding(value));
    }
#endif
    return u;
}

QByteArray buildFormData(const QHash<QString, QString>& params)
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    QUrlQuery query;
    Q_FOREACH (const QString& key, params.keys()) {
        QString value = params[key];
        query.addQueryItem(QUrl::toPercentEncoding(key),
                           QUrl::toPercentEncoding(value));

    }
    return query.query(QUrl::FullyEncoded).toUtf8();
#else
    QUrl u;
    Q_FOREACH (const QString& key, params.keys()) {
        QString value = params[key];
        u.addEncodedQueryItem(QUrl::toPercentEncoding(key),
                              QUrl::toPercentEncoding(value));
    }
    return u.encodedQuery();
#endif
}
