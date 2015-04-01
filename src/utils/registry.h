#ifndef SEAFILE_CLIENT_UTILS_REGISTRY_H
#define SEAFILE_CLIENT_UTILS_REGISTRY_H

#include <QString>
#include <windows.h>

#ifndef KEY_WOW64_64KEY
#define KEY_WOW64_64KEY 0x0100
#endif

#ifndef KEY_WOW64_32KEY
#define KEY_WOW64_32KEY 0x0200
#endif

/**
 * Windows Registry Element
 */
class RegElement {
public:
    static void removeRegKey(const QString& key);

    RegElement(const HKEY& root,
               const QString& path,
               const QString& name,
               const QString& value,
               bool expand=false);

    RegElement(const HKEY& root,
               const QString& path,
               const QString& name,
               DWORD value);

    int add();
    void remove();
    bool exists();

    const HKEY& root() const { return root_; }
    const QString& path() const { return path_; }
    const QString& name() const { return name_; }
    const QString& stringValue() const { return string_value_; }
    DWORD dwordValue() const { return dword_value_; }

public:
    static int removeRegKey(HKEY root, const QString& path, const QString& subkey);

private:

    int openParentKey(HKEY *pKey);

    HKEY root_;
    QString path_;
    QString name_;
    QString string_value_;
    DWORD dword_value_;
    DWORD type_;
};

#endif // SEAFILE_CLIENT_UTILS_REGISTRY_H
