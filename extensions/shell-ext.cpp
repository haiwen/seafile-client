// Initialize GUIDs (should be done only and at-least once per DLL/EXE)
#include <initguid.h>
#include "guids.h"

#include "ext-common.h"
#include "shell-ext.h"


// *********************** CShellExt *************************
CShellExt::CShellExt()
{
    m_cRef = 0L;
    InterlockedIncrement(&g_cRefThisDll);

    // INITCOMMONCONTROLSEX used = {
    //     sizeof(INITCOMMONCONTROLSEX),
    //         ICC_LISTVIEW_CLASSES | ICC_WIN95_CLASSES | ICC_BAR_CLASSES | ICC_USEREX_CLASSES
    // };
    // InitCommonControlsEx(&used);
}

CShellExt::~CShellExt()
{
    InterlockedDecrement(&g_cRefThisDll);
}

STDMETHODIMP CShellExt::QueryInterface(REFIID riid, LPVOID FAR *ppv)
{
    if (ppv == 0)
        return E_POINTER;

    *ppv = NULL;

    if (IsEqualIID(riid, IID_IShellExtInit) || IsEqualIID(riid, IID_IUnknown))
    {
        *ppv = static_cast<LPSHELLEXTINIT>(this);
    }
    // else if (IsEqualIID(riid, IID_IContextMenu))
    // {
    //     *ppv = static_cast<LPCONTEXTMENU>(this);
    // }
    // else if (IsEqualIID(riid, IID_IContextMenu2))
    // {
    //     *ppv = static_cast<LPCONTEXTMENU2>(this);
    // }
    // else if (IsEqualIID(riid, IID_IContextMenu3))
    // {
    //     *ppv = static_cast<LPCONTEXTMENU3>(this);
    // }
    else
    {
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG) CShellExt::AddRef()
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CShellExt::Release()
{
    if (--m_cRef)
        return m_cRef;

    delete this;

    return 0L;
}

