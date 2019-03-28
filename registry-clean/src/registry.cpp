#include <QtGlobal>
#include <QStringList>
#include <shlwapi.h>
#include "registry.h"

namespace {

#if defined (Q_OS_WIN32)
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

int removeRegKey(HKEY root, const QString& path, const QString& subkey)
{
    HKEY hKey;
    LONG result = RegOpenKeyExW(root,
                                path.toStdWString().c_str(),
                                0L,
                                KEY_ALL_ACCESS|KEY_WOW64_64KEY,
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
#endif

} //namespace

#if defined(Q_OS_WIN32)
bool cleanRegistryItem(HKEY root, const QString path)
{
    HKEY parent_key;
    QStringList keylist;
    LONG result = openKey(root, path, &parent_key, KEY_READ|KEY_WOW64_64KEY);
    if(result != ERROR_SUCCESS)
    {
        qDebug("open register item error");
        return false;
    }

    DWORD lpcSubKeys = 0;
	DWORD lpcbMaxSubKeyLen = 0;
    // ReQuery registry subkey numbers.
    result=RegQueryInfoKeyW(parent_key,
                            NULL,
                            NULL,
                            NULL,
                            &lpcSubKeys,
                            &lpcbMaxSubKeyLen,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            NULL);

    if(result!=ERROR_SUCCESS)
    {
        qDebug("regquery info key failed");
        return false;
    }

    DWORD lpcchName = 1024;
    wchar_t lpName[1024] = L"";
    for(DWORD dwIndex=0; dwIndex<lpcSubKeys; ++dwIndex)
    {
        RegEnumKeyExW(parent_key,
                      dwIndex,
                      lpName,
                      &lpcchName,
                      NULL,
                      NULL,
                      NULL,
                      NULL);

        keylist.append(QString::fromWCharArray(lpName));
        memset(lpName, 0, sizeof(lpName));
        lpcchName = 1024;
    }
    RegCloseKey(parent_key);

    for(int i = 0; i < keylist.size(); i++)
    {
        qDebug("key item is %s \n", keylist[i].toUtf8().data());
        if (keylist[i].contains("Seafile") || keylist[i].contains("OneDrive")) {

        } else {
            int result = removeRegKey(root, path, keylist[i]);
            qDebug("remove keylist %s, result is %d \n", keylist[i].toUtf8().data(), result);
        }

    }

}
#endif
