#include "ext-common.h"
#include "ext-utils.h"
#include "shell-ext.h"
#include "log.h"
#include "commands.h"

#include <string>

namespace utils = seafile::utils;

namespace {

bool shouldIgnorePath(const std::string& path)
{
    /* Show no menu for drive root, such as C: D: */
    if (path.size() <= 3) {
        return TRUE;
    }

    /* Ignore flash disk, network mounted drive, etc. */
    if (GetDriveType(path.substr(0, 3).c_str()) != DRIVE_FIXED) {
        return TRUE;
    }

    return FALSE;
}

const char *kMainMenuName = "Seafile";

}


STDMETHODIMP ShellExt::Initialize(LPCITEMIDLIST pIDFolder,
                                   LPDATAOBJECT pDataObj,
                                   HKEY  hRegKey)
{
    return Initialize_Wrap(pIDFolder, pDataObj, hRegKey);
}

STDMETHODIMP ShellExt::Initialize_Wrap(LPCITEMIDLIST folder,
                                        LPDATAOBJECT data,
                                        HKEY /* hRegKey */)
{
    FORMATETC format = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    STGMEDIUM stg = {TYMED_HGLOBAL};
    HDROP drop;
    UINT count;
    HRESULT result = S_OK;
    char path[MAX_PATH] = {0};

    /* 'folder' param is not null only when clicking at the foler background;
       When right click on a file, it's NULL */
    if (folder) {
        SHGetPathFromIDList(folder, path);
        path_ = utils::normalizedPath(utils::localeToUtf8(path));
    }

    /* if 'data' is NULL, then it's a background click, we have set
     * this_->name to folder's name above, and the Init work is done */
    if (!data)
        return S_OK;

    /* 'data' is no null, which means we are operating on a file. The
     * following lines until the end of the function is used to extract the
     * filename of the current file. */
    if (FAILED(data->GetData(&format, &stg)))
        return E_INVALIDARG;

    drop = (HDROP)GlobalLock(stg.hGlobal);
    if (!drop)
        return E_INVALIDARG;

    count = DragQueryFile(drop, 0xFFFFFFFF, NULL, 0);
    if (count == 0)
        result = E_INVALIDARG;
    else if (!DragQueryFile(drop, 0, path, sizeof(path)))
        result = E_INVALIDARG;

    path_ = utils::normalizedPath(utils::localeToUtf8(path));
    GlobalUnlock(stg.hGlobal);
    ReleaseStgMedium(&stg);

    return result;
}

STDMETHODIMP ShellExt::QueryContextMenu(HMENU hMenu,
                                         UINT indexMenu,
                                         UINT idCmdFirst,
                                         UINT idCmdLast,
                                         UINT uFlags)
{
    return QueryContextMenu_Wrap(hMenu, indexMenu, idCmdFirst, idCmdLast, uFlags);
}


STDMETHODIMP ShellExt::QueryContextMenu_Wrap(HMENU menu,
                                              UINT indexMenu,
                                              UINT first_command,
                                              UINT last_command,
                                              UINT flags)
{
    seaf_ext_log("QueryContextMenu called on %s", path_.c_str());
    /* do nothing when user is double clicking */
    if (flags & CMF_DEFAULTONLY)
        return S_OK;

    if (shouldIgnorePath(path_)) {
        seaf_ext_log("ignore context menu for %s", path_.c_str());
        return S_OK;
    }

    std::string path_in_repo;
    if (!pathInRepo(path_, &path_in_repo) || path_in_repo.size() <= 1) {
        return S_OK;
    }

    main_menu_ = menu;
    first_ = first_command;
    last_ = last_command;
    index_ = 0;

    buildSubMenu();

    if (!insertMainMenu()) {
        return S_FALSE;
    }

    return MAKE_HRESULT(
        SEVERITY_SUCCESS, FACILITY_NULL, 3 + next_active_item_);
}

void ShellExt::tweakMenu(HMENU menu)
{
    MENUINFO MenuInfo;
    MenuInfo.cbSize  = sizeof(MenuInfo);
    MenuInfo.fMask   = MIM_STYLE | MIM_APPLYTOSUBMENUS;
    MenuInfo.dwStyle = MNS_CHECKORBMP;

    SetMenuInfo(menu, &MenuInfo);
}

STDMETHODIMP ShellExt::InvokeCommand(LPCMINVOKECOMMANDINFO lpcmi)
{
    seaf_ext_log("InvokeCommand called");
    return InvokeCommand_Wrap(lpcmi);
}

// This is called when you invoke a command on the menu
STDMETHODIMP ShellExt::InvokeCommand_Wrap(LPCMINVOKECOMMANDINFO lpcmi)
{
    seafile::GetShareLinkCommand cmd(path_);
    cmd.send();
    return S_OK;
}

STDMETHODIMP ShellExt::GetCommandString(UINT_PTR idCmd,
                                         UINT flags,
                                         UINT FAR * reserved,
                                         LPSTR pszName,
                                         UINT cchMax)
{
    return GetCommandString_Wrap(idCmd, flags, reserved, pszName, cchMax);
}

// This is for the status bar and things like that:
STDMETHODIMP ShellExt::GetCommandString_Wrap(UINT_PTR idCmd,
                                              UINT flags,
                                              UINT FAR * /*reserved*/,
                                              LPSTR pszName,
                                              UINT cchMax)
{
    lstrcpynW((LPWSTR)pszName, L"This is Seafile help string.", cchMax);
    return S_OK;
}

STDMETHODIMP ShellExt::HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return HandleMenuMsg_Wrap(uMsg, wParam, lParam);
}

STDMETHODIMP ShellExt::HandleMenuMsg_Wrap(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT res;
    return HandleMenuMsg2(uMsg, wParam, lParam, &res);
}

STDMETHODIMP ShellExt::HandleMenuMsg2(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *pResult)
{
    return HandleMenuMsg2_Wrap(uMsg, wParam, lParam, pResult);
}

STDMETHODIMP ShellExt::HandleMenuMsg2_Wrap(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *pResult)
{
    return S_OK;
}

/**
 * Add two menu seperators, with seafile menu between them
 */
bool ShellExt::insertMainMenu()
{
    // Insert a seperate before seafile menu
    if (!InsertMenu(main_menu_, index_++, MF_BYPOSITION |MF_SEPARATOR, 0, ""))
        return FALSE;

    MENUITEMINFO menuiteminfo;
    ZeroMemory(&menuiteminfo, sizeof(menuiteminfo));
    menuiteminfo.cbSize = sizeof(menuiteminfo);
    menuiteminfo.fMask = MIIM_FTYPE | MIIM_SUBMENU | MIIM_BITMAP | MIIM_STRING | MIIM_ID;
    menuiteminfo.fType = MFT_STRING;
    menuiteminfo.dwTypeData = (char*)kMainMenuName;
    menuiteminfo.cch = strlen(kMainMenuName);
    menuiteminfo.hbmpItem = HBMMENU_CALLBACK;
    menuiteminfo.hSubMenu = sub_menu_;
    menuiteminfo.wID = first_;

    if (!InsertMenuItem(main_menu_, index_++, TRUE, &menuiteminfo))
        return FALSE;

    // Insert a seperate after seafile menu
    if (!InsertMenu(main_menu_, index_++, MF_BYPOSITION |MF_SEPARATOR, 0, ""))
        return FALSE;

    /* Set menu styles of submenu */
    tweakMenu(main_menu_);

    return TRUE;
}

void ShellExt::buildSubMenu()
{
    MENUITEMINFO minfo = {0};
    minfo.cbSize = sizeof(MENUITEMINFO);
    minfo.fMask = MIIM_FTYPE | MIIM_BITMAP | MIIM_STRING | MIIM_ID;
    minfo.fType = MFT_STRING;
    std::string name("get share link");
    minfo.dwTypeData = (char *)name.c_str();
    minfo.cch = name.size();
    minfo.hbmpItem = HBMMENU_CALLBACK;
    /* menu.first is used by main menu item "seafile"   */
    minfo.wID = first_ + 1 + next_active_item_++;

    InsertMenuItem (sub_menu_, /* menu */
                    index_++,  /* position */
                    TRUE,      /* by position */
                    &minfo);
}
