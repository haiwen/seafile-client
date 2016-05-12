#ifndef SEAFILE_EXT_SHELL_EXT_H
#define SEAFILE_EXT_SHELL_EXT_H

#include <string>
#include <memory>
#include <vector>
#include "commands.h"
#include "ext-utils.h"

extern  volatile LONG       g_cRefThisDll;          // Reference count of this DLL.
extern  HINSTANCE           g_hmodThisDll;          // Instance handle for this DLL
extern  HINSTANCE           g_hResInst;

struct SeafileMenuItem {
    std::string text;           /* displayed in the menu */
    std::string help;           /* displayed in the status bar */
};

class ShellExt : public IContextMenu3,
                 IShellIconOverlayIdentifier,
                 IShellExtInit
{

protected:
    ULONG                           m_cRef;

    // IContextMenu2 wrapper functions to catch exceptions and send crash reports
    STDMETHODIMP    QueryContextMenu_Wrap(HMENU hMenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags);
    STDMETHODIMP    InvokeCommand_Wrap(LPCMINVOKECOMMANDINFO lpcmi);
    STDMETHODIMP    GetCommandString_Wrap(UINT_PTR idCmd, UINT uFlags, UINT FAR *reserved, LPSTR pszName, UINT cchMax);
    STDMETHODIMP    HandleMenuMsg_Wrap(UINT uMsg, WPARAM wParam, LPARAM lParam);

    // IContextMenu3 wrapper functions to catch exceptions and send crash reports
    STDMETHODIMP    HandleMenuMsg2_Wrap(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *pResult);

    // IShellExtInit wrapper functions to catch exceptions and send crash reports
    STDMETHODIMP    Initialize_Wrap(LPCITEMIDLIST pIDFolder, LPDATAOBJECT pDataObj, HKEY hKeyID);

    seafile::RepoInfo::Status status_;

public:
    ShellExt(seafile::RepoInfo::Status status = seafile::RepoInfo::NoStatus);
    virtual ~ShellExt();

    // IUnknown members
    STDMETHODIMP         QueryInterface(REFIID, LPVOID FAR *);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IContextMenu2 members
    STDMETHODIMP    QueryContextMenu(HMENU hMenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags);
    STDMETHODIMP    InvokeCommand(LPCMINVOKECOMMANDINFO lpcmi);
    STDMETHODIMP    GetCommandString(UINT_PTR idCmd, UINT uFlags, UINT FAR *reserved, LPSTR pszName, UINT cchMax);
    STDMETHODIMP    HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam);

    // IContextMenu3 members
    STDMETHODIMP    HandleMenuMsg2(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *pResult);

    // IShellIconOverlayIdentifier members
    STDMETHODIMP    GetOverlayInfo(LPWSTR pwszIconFile, int cchMax, int *pIndex, DWORD *pdwFlags);
    STDMETHODIMP    GetPriority(int *pPriority);
    STDMETHODIMP    IsMemberOf(LPCWSTR pwszPath, DWORD dwAttrib);

     // IShellExtInit methods
    STDMETHODIMP    Initialize(LPCITEMIDLIST pIDFolder, LPDATAOBJECT pDataObj, HKEY hKeyID);

private:
    enum MenuOp {
        GetShareLink,
        GetInternalLink,
        LockFile,
        UnlockFile,
        ShareToUser,
        ShareToGroup,
        ShowHistory,
    };

    void buildSubMenu(const seafile::RepoInfo& repo,
                      const std::string& path_in_repo);
    MENUITEMINFO createMenuItem(const std::string& text);
    bool insertMainMenu();
    void insertSubMenuItem(const std::string& text, MenuOp op);
    void tweakMenu(HMENU menu);

    bool getReposList(seafile::RepoInfoList *wts);
    bool pathInRepo(const std::string& path, std::string *path_in_repo, seafile::RepoInfo *repo=0);
    bool isRepoTopDir(const std::string& path);
    seafile::RepoInfo getRepoInfoByPath(const std::string& path);
    seafile::RepoInfo::Status getRepoFileStatus(const std::string& repo_id,
                                                const std::string& path_in_repo,
                                                bool isdir);
    /* the file/dir current clicked on */
    std::string path_;

    static std::unique_ptr<seafile::RepoInfoList> repos_cache_;
    static uint64_t cache_ts_;
    seafile::utils::Mutex repos_cache_mutex_;

    /* The main menu */
    HMENU main_menu_;
    /* The popup seafile submenu */
    HMENU sub_menu_;
    UINT index_;
    UINT first_;
    UINT last_;
    UINT next_active_item_;

    std::vector<MenuOp> active_menu_items_;
};

#endif // SEAFILE_EXT_SHELL_EXT_H
