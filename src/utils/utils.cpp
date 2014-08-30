
#include <cassert>
#include <errno.h>
#include <dirent.h>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <sqlite3.h>
#include <glib.h>
#include <cstring>
#include <QString>
#include <string>
#include <jansson.h>

#if defined(Q_WS_MAC)
    #include <sys/sysctl.h>
    #include "utils-mac.h"
#elif defined(Q_WS_WIN)
    #include <windows.h>
    #include <psapi.h>
#endif

#include <QMap>
#include <QVariant>
#include <QDebug>
#include <QDateTime>
#include <QCryptographicHash>

#include "seafile-applet.h"

#include "utils.h"


namespace {

#if defined(Q_WS_WIN)
const char *kCcnetConfDir = "ccnet";
#else
const char *kCcnetConfDir = ".ccnet";
#endif

const char *kSettingHideDockIcon = "hideDockIcon";

} // namespace


QString defaultCcnetDir() {
    const char *env = g_getenv("CCNET_CONF_DIR");
    if (env) {
        return QString::fromUtf8(env);
    } else {
        return QDir::home().filePath(kCcnetConfDir);
    }
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
#if defined(Q_WS_WIN)
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


#if defined(Q_WS_WIN)
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
        qDebug("Failed to open Registry key %s\n", key_run);
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
        qDebug("Failed to create auto start value\n");
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
        qDebug("Failed to remove auto start value");
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

bool
get_seafile_hide_dock_icon(void)
{
#if defined(Q_WS_MAC)
    return __mac_getDefault(kSettingHideDockIcon);
#endif
    return false;
}

int
set_seafile_hide_dock_icon(bool on)
{
#if defined(Q_WS_MAC)
    __mac_setDefault(kSettingHideDockIcon, on);
    __mac_setDockIconStyle(on);
#endif
    return 0;
}

int
init_seafile_hide_dock_icon(void)
{
#if defined(Q_WS_MAC)
    __mac_initDefaults(kSettingHideDockIcon);
    __mac_setDockIconStyle(__mac_getDefault(kSettingHideDockIcon));
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
        if (json_is_string(value)) {
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
        b = b.right(1);
    }
    return QUrl(a + b);
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
    QString s;
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

    QString s;
    QString s_none = QObject::tr("<Not Part of Certificate>");
    #define CERTIFICATE_STR(x) ( ((x) == "" ) ? s_none : (x) )

    s += "\nIssued To\n";
    s += "CommonName(CN):             " + CERTIFICATE_STR(cert.subjectInfo(QSslCertificate::CommonName)) + "\n";
    s += "Organization(O):            " + CERTIFICATE_STR(cert.subjectInfo(QSslCertificate::Organization)) + "\n";
    s += "OrganizationalUnitName(OU): " + CERTIFICATE_STR(cert.subjectInfo(QSslCertificate::OrganizationalUnitName)) + "\n";
    s += "Serial Number:              " + CERTIFICATE_STR(dumpHexPresentation(cert.serialNumber())) + "\n";

    s += "\nIssued By\n";
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
