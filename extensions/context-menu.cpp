#include "ext-common.h"
#include "shell-ext.h"

#include <string>


STDMETHODIMP CShellExt::Initialize(LPCITEMIDLIST pIDFolder,
                                   LPDATAOBJECT pDataObj,
                                   HKEY  hRegKey)
{
    return Initialize_Wrap(pIDFolder, pDataObj, hRegKey);
}

STDMETHODIMP CShellExt::Initialize_Wrap(LPCITEMIDLIST pIDFolder,
                                        LPDATAOBJECT pDataObj,
                                        HKEY /* hRegKey */)
{
    return S_OK;
}

STDMETHODIMP CShellExt::QueryContextMenu(HMENU hMenu,
                                         UINT indexMenu,
                                         UINT idCmdFirst,
                                         UINT idCmdLast,
                                         UINT uFlags)
{
    return QueryContextMenu_Wrap(hMenu, indexMenu, idCmdFirst, idCmdLast, uFlags);
}


STDMETHODIMP CShellExt::QueryContextMenu_Wrap(HMENU hMenu,
                                              UINT indexMenu,
                                              UINT idCmdFirst,
                                              UINT /*idCmdLast*/,
                                              UINT uFlags)
{
    if ((uFlags & CMF_DEFAULTONLY)!=0)
        return S_OK;                    //we don't change the default action

    if (((uFlags & 0x000f)!=CMF_NORMAL)&&(!(uFlags & CMF_EXPLORE))&&(!(uFlags & CMF_VERBSONLY)))
        return S_OK;

    return S_OK;
}

// void CShellExt::TweakMenu(HMENU hMenu)
// {
//     MENUINFO MenuInfo = {};
//     MenuInfo.cbSize  = sizeof(MenuInfo);
//     MenuInfo.fMask   = MIM_STYLE | MIM_APPLYTOSUBMENUS;
//     MenuInfo.dwStyle = MNS_CHECKORBMP;

//     SetMenuInfo(hMenu, &MenuInfo);
// }

STDMETHODIMP CShellExt::InvokeCommand(LPCMINVOKECOMMANDINFO lpcmi)
{
    return InvokeCommand_Wrap(lpcmi);
}

// This is called when you invoke a command on the menu:
STDMETHODIMP CShellExt::InvokeCommand_Wrap(LPCMINVOKECOMMANDINFO lpcmi)
{
    return S_OK;
}

STDMETHODIMP CShellExt::GetCommandString(UINT_PTR idCmd,
                                         UINT uFlags,
                                         UINT FAR * reserved,
                                         LPSTR pszName,
                                         UINT cchMax)
{
    return GetCommandString_Wrap(idCmd, uFlags, reserved, pszName, cchMax);
}

// This is for the status bar and things like that:
STDMETHODIMP CShellExt::GetCommandString_Wrap(UINT_PTR idCmd,
                                              UINT uFlags,
                                              UINT FAR * /*reserved*/,
                                              LPSTR pszName,
                                              UINT cchMax)
{
    lstrcpynW((LPWSTR)pszName, L"This is Seafile help string.", cchMax);
    return S_OK;
}

STDMETHODIMP CShellExt::HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return HandleMenuMsg_Wrap(uMsg, wParam, lParam);
}

STDMETHODIMP CShellExt::HandleMenuMsg_Wrap(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT res;
    return HandleMenuMsg2(uMsg, wParam, lParam, &res);
}

STDMETHODIMP CShellExt::HandleMenuMsg2(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *pResult)
{
    return HandleMenuMsg2_Wrap(uMsg, wParam, lParam, pResult);
}

STDMETHODIMP CShellExt::HandleMenuMsg2_Wrap(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *pResult)
{
    return S_OK;
}
