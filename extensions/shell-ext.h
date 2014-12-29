#ifndef SEAFILE_EXT_SHELL_EXT_H
#define SEAFILE_EXT_SHELL_EXT_H

extern  volatile LONG       g_cRefThisDll;          // Reference count of this DLL.
extern  HINSTANCE           g_hmodThisDll;          // Instance handle for this DLL
extern  HINSTANCE           g_hResInst;

class CShellExt : public IContextMenu3,
                         IShellExtInit
{
protected:
    ULONG                           m_cRef;

    /** \name IContextMenu2 wrappers
     * IContextMenu2 wrapper functions to catch exceptions and send crash reports
     */
    //@{
    STDMETHODIMP    QueryContextMenu_Wrap(HMENU hMenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags);
    STDMETHODIMP    InvokeCommand_Wrap(LPCMINVOKECOMMANDINFO lpcmi);
    STDMETHODIMP    GetCommandString_Wrap(UINT_PTR idCmd, UINT uFlags, UINT FAR *reserved, LPSTR pszName, UINT cchMax);
    STDMETHODIMP    HandleMenuMsg_Wrap(UINT uMsg, WPARAM wParam, LPARAM lParam);
    //@}

    /** \name IContextMenu3 wrappers
     * IContextMenu3 wrapper functions to catch exceptions and send crash reports
     */
    //@{
    STDMETHODIMP    HandleMenuMsg2_Wrap(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *pResult);
    //@}

    /** \name IShellExtInit wrappers
     * IShellExtInit wrapper functions to catch exceptions and send crash reports
     */
    //@{
    STDMETHODIMP    Initialize_Wrap(LPCITEMIDLIST pIDFolder, LPDATAOBJECT pDataObj, HKEY hKeyID);
    //@}

public:
    CShellExt();
    virtual ~CShellExt();

    /** \name IUnknown
     * IUnknown members
     */
    //@{
    STDMETHODIMP         QueryInterface(REFIID, LPVOID FAR *);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();
    //@}

    /** \name IContextMenu2
     * IContextMenu2 members
     */
    //@{
    STDMETHODIMP    QueryContextMenu(HMENU hMenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags);
    STDMETHODIMP    InvokeCommand(LPCMINVOKECOMMANDINFO lpcmi);
    STDMETHODIMP    GetCommandString(UINT_PTR idCmd, UINT uFlags, UINT FAR *reserved, LPSTR pszName, UINT cchMax);
    STDMETHODIMP    HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam);
    //@}

    /** \name IContextMenu3
     * IContextMenu3 members
     */
    //@{
    STDMETHODIMP    HandleMenuMsg2(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *pResult);
    //@}

    /** \name IShellExtInit
     * IShellExtInit methods
     */
    //@{
    STDMETHODIMP    Initialize(LPCITEMIDLIST pIDFolder, LPDATAOBJECT pDataObj, HKEY hKeyID);
    //@}

};

#endif // SEAFILE_EXT_SHELL_EXT_H
