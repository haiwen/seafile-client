#ifndef SEAFILE_EXT_SHELL_EXT_H
#define SEAFILE_EXT_SHELL_EXT_H

#include <string>

extern  volatile LONG       g_cRefThisDll;          // Reference count of this DLL.
extern  HINSTANCE           g_hmodThisDll;          // Instance handle for this DLL
extern  HINSTANCE           g_hResInst;

struct SeafileMenuItem {
    std::string text;           /* displayed in the menu */
    std::string help;           /* displayed in the status bar */
};

class ShellExt : public IContextMenu3,
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

public:
    ShellExt();
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

     // IShellExtInit methods
    STDMETHODIMP    Initialize(LPCITEMIDLIST pIDFolder, LPDATAOBJECT pDataObj, HKEY hKeyID);

private:
    void buildSubMenu();
    bool insertMainMenu();
    void tweakMenu(HMENU menu);

    bool pathInRepo(const std::string path, std::string *path_in_repo);

    /* the file/dir current clicked on */
    std::string path_;

    /* The main menu */
    HMENU main_menu_;
    /* The popup seafile submenu */
    HMENU sub_menu_;
    UINT index_;
    UINT first_;
    UINT last_;
    UINT next_active_item_;
    // struct menu_item *active_menu;
    // bool add_sep;

    // unsigned int count;
    // unsigned int selection;

    /* non-empety if in a repo dir */
    std::string repo_id;
    /* repo top wt, non-empty if in a repo dir */
    std::string repo_wt;
};

#endif // SEAFILE_EXT_SHELL_EXT_H
