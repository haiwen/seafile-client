#include <windows.h>
#include <shlwapi.h>
#include <vector>

#include "utils/stl.h"
#include "utils/utils.h"


#include "registry.h"

namespace {

LONG openKey(HKEY root, const QString& path, HKEY *p_key, REGSAM samDesired = KEY_ALL_ACCESS)
{
    LONG result;
    result = RegOpenKeyExW(root,
                           path.toStdWString().c_str(),
                           0L,
                           samDesired,
                           p_key);

    return result;
}

QString softwareSeafile()
{
    return QString("SOFTWARE\\%1").arg(getBrand());
}

} // namespace

RegElement::RegElement(const HKEY& root, const QString& path,
                       const QString& name, const QString& value, bool expand)
    : root_(root),
      path_(path),
      name_(name),
      string_value_(value),
      dword_value_(0),
      type_(expand ? REG_EXPAND_SZ : REG_SZ)
{
}

RegElement::RegElement(const HKEY& root, const QString& path,
                       const QString& name, DWORD value)
    : root_(root),
      path_(path),
      name_(name),
      string_value_(""),
      dword_value_(value),
      type_(REG_DWORD)
{
}

int RegElement::openParentKey(HKEY *pKey)
{
    DWORD disp;
    HRESULT result;

    result = RegCreateKeyExW (root_,
                              path_.toStdWString().c_str(),
                              0, NULL,
                              REG_OPTION_NON_VOLATILE,
                              KEY_WRITE | KEY_WOW64_64KEY,
                              NULL,
                              pKey,
                              &disp);

    if (result != ERROR_SUCCESS) {
        return -1;
    }

    return 0;
}

int RegElement::add()
{
    HKEY parent_key;
    DWORD value_len;
    LONG result;

    if (openParentKey(&parent_key) < 0) {
        return -1;
    }

    if (type_ == REG_SZ || type_ == REG_EXPAND_SZ) {
        // See http://msdn.microsoft.com/en-us/library/windows/desktop/ms724923(v=vs.85).aspx
        value_len = sizeof(wchar_t) * (string_value_.toStdWString().length() + 1);
        result = RegSetValueExW (parent_key,
                                 name_.toStdWString().c_str(),
                                 0, REG_SZ,
                                 (const BYTE *)(string_value_.toStdWString().c_str()),
                                 value_len);
    } else {
        value_len = sizeof(dword_value_);
        result = RegSetValueExW (parent_key,
                                 name_.toStdWString().c_str(),
                                 0, REG_DWORD,
                                 (const BYTE *)&dword_value_,
                                 value_len);
    }

    if (result != ERROR_SUCCESS) {
        return -1;
    }

    return 0;
}

int RegElement::removeRegKey(HKEY root, const QString& path, const QString& subkey)
{
    HKEY hKey;
    LONG result = RegOpenKeyExW(root,
                                path.toStdWString().c_str(),
                                0L,
                                KEY_ALL_ACCESS,
                                &hKey);

    if (result != ERROR_SUCCESS) {
        return -1;
    }

    result = SHDeleteKeyW(hKey, subkey.toStdWString().c_str());
    if (result != ERROR_SUCCESS) {
        return -1;
    }

    return 0;
}

bool RegElement::exists()
{
    HKEY parent_key;
    LONG result = openKey(root_, path_, &parent_key, KEY_READ);
    if (result != ERROR_SUCCESS) {
        return false;
    }

    char buf[MAX_PATH] = {0};
    DWORD len = sizeof(buf);
    result = RegQueryValueExW (parent_key,
                               name_.toStdWString().c_str(),
                               NULL,             /* reserved */
                               NULL,             /* output type */
                               (LPBYTE)buf,      /* output data */
                               &len);            /* output length */

    RegCloseKey(parent_key);
    if (result != ERROR_SUCCESS) {
        return false;
    }

    return true;
}

void RegElement::read()
{
    string_value_.clear();
    dword_value_ = 0;
    HKEY parent_key;
    LONG result = openKey(root_, path_, &parent_key, KEY_READ);
    if (result != ERROR_SUCCESS) {
        return;
    }
    const std::wstring std_name = name_.toStdWString();

    DWORD len;
    // get value size
    result = RegQueryValueExW (parent_key,
                               std_name.c_str(),
                               NULL,             /* reserved */
                               &type_,           /* output type */
                               NULL,             /* output data */
                               &len);            /* output length */
    if (result != ERROR_SUCCESS) {
        RegCloseKey(parent_key);
        return;
    }
    // get value
#ifndef UTILS_CXX11_MODE
    std::vector<wchar_t> buf;
#else
    utils::WBufferArray buf;
#endif
    buf.resize(len);
    result = RegQueryValueExW (parent_key,
                               std_name.c_str(),
                               NULL,             /* reserved */
                               &type_,           /* output type */
                               (LPBYTE)&buf[0],  /* output data */
                               &len);            /* output length */
    buf.resize(len);
    if (result != ERROR_SUCCESS) {
        RegCloseKey(parent_key);
        return;
    }

    switch (type_) {
        case REG_EXPAND_SZ:
        case REG_SZ:
            {
                // expand environment strings
                wchar_t expanded_buf[MAX_PATH];
                DWORD size = ExpandEnvironmentStringsW(&buf[0], expanded_buf, MAX_PATH);
                if (size == 0 || size > MAX_PATH)
                    string_value_ = QString::fromWCharArray(&buf[0], buf.size());
                else
                    string_value_ = QString::fromWCharArray(expanded_buf, size);
            }
            break;
        case REG_NONE:
        case REG_BINARY:
            string_value_ = QString::fromWCharArray(&buf[0], buf.size() / 2);
            break;
        case REG_DWORD_BIG_ENDIAN:
        case REG_DWORD:
            if (buf.size() != sizeof(int))
                return;
            memcpy((char*)&dword_value_, buf.data(), sizeof(int));
            break;
        case REG_QWORD: {
            if (buf.size() != sizeof(int))
                return;
            qint64 value;
            memcpy((char*)&value, buf.data(), sizeof(int));
            dword_value_ = (int)value;
            break;
        }
        case REG_MULTI_SZ:
        default:
          break;
    }

    RegCloseKey(parent_key);

    // workaround with a bug
    string_value_ = QString::fromUtf8(string_value_.toUtf8());

    return;
}

void RegElement::remove()
{
    HKEY parent_key;
    LONG result = openKey(root_, path_, &parent_key, KEY_ALL_ACCESS);
    if (result != ERROR_SUCCESS) {
        return;
    }
    result = RegDeleteValueW (parent_key, name_.toStdWString().c_str());
    RegCloseKey(parent_key);
}

QVariant RegElement::value() const
{
    if (type_ == REG_SZ || type_ == REG_EXPAND_SZ
        || type_ == REG_NONE || type_ == REG_BINARY) {
        return string_value_;
    } else {
        return int(dword_value_);
    }
}

int RegElement::getIntValue(HKEY root,
                            const QString& path,
                            const QString& name,
                            bool *exists,
                            int default_val)
{
    RegElement reg(root, path, name, "");
    if (!reg.exists()) {
        if (exists) {
            *exists = false;
        }
        return default_val;
    }
    if (exists) {
        *exists = true;
    }
    reg.read();

    if (!reg.stringValue().isEmpty())
        return reg.stringValue().toInt();

    return reg.dwordValue();
}

int RegElement::getPreconfigureIntValue(const QString& name)
{
    bool exists;
    int ret = getIntValue(
        HKEY_CURRENT_USER, softwareSeafile(), name, &exists);
    if (exists) {
        return ret;
    }

    return RegElement::getIntValue(
        HKEY_LOCAL_MACHINE, softwareSeafile(), name);
}

QString RegElement::getStringValue(HKEY root,
                                   const QString& path,
                                   const QString& name,
                                   bool *exists,
                                   QString default_val)
{
    RegElement reg(root, path, name, "");
    if (!reg.exists()) {
        if (exists) {
            *exists = false;
        }
        return default_val;
    }
    if (exists) {
        *exists = true;
    }
    reg.read();
    return reg.stringValue();
}

QString RegElement::getPreconfigureStringValue(const QString& name)
{
    bool exists;
    QString ret = getStringValue(
        HKEY_CURRENT_USER, softwareSeafile(), name, &exists);
    if (exists) {
        return ret;
    }

    return RegElement::getStringValue(
        HKEY_LOCAL_MACHINE, softwareSeafile(), name);
}

QVariant RegElement::getPreconfigureValue(const QString& name)
{
    QVariant v = getValue(HKEY_CURRENT_USER, softwareSeafile(), name);
    return v.isNull() ? getValue(HKEY_LOCAL_MACHINE, softwareSeafile(), name) : v;
}

QVariant RegElement::getValue(HKEY root,
                              const QString& path,
                              const QString& name)
{
    RegElement reg(root, path, name, "");
    if (!reg.exists()) {
        return QVariant();
    }
    reg.read();

    return reg.value();
}
