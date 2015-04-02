
#include "ext-common.h"

#include "class-factory.h"
#include "shell-ext.h"

ShellExtClassFactory::ShellExtClassFactory()
{
    m_cRef = 0L;

    InterlockedIncrement(&g_cRefThisDll);
}

ShellExtClassFactory::~ShellExtClassFactory()
{
    InterlockedDecrement(&g_cRefThisDll);
}

STDMETHODIMP ShellExtClassFactory::QueryInterface(REFIID riid,
                                                   LPVOID FAR *ppv)
{
    if (ppv == 0)
        return E_POINTER;

    *ppv = NULL;

    // Any interface on this object is the object pointer

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IClassFactory))
    {
        *ppv = static_cast<LPCLASSFACTORY>(this);
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) ShellExtClassFactory::AddRef()
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) ShellExtClassFactory::Release()
{
    if (--m_cRef)
        return m_cRef;

    delete this;

    return 0L;
}

STDMETHODIMP ShellExtClassFactory::CreateInstance(LPUNKNOWN pUnkOuter,
                                                   REFIID riid,
                                                   LPVOID *ppvObj)
{
    if(ppvObj == 0)
        return E_POINTER;

    *ppvObj = NULL;

    // Shell extensions typically don't support aggregation (inheritance)

    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    // Create the main shell extension object.  The shell will then call
    // QueryInterface with IID_IShellExtInit--this is how shell extensions are
    // initialized.

    ShellExt* pShellExt = new ShellExt; // Create the ShellExt object

    const HRESULT hr = pShellExt->QueryInterface(riid, ppvObj);
    if(FAILED(hr))
        delete pShellExt;
    return hr;
}

STDMETHODIMP ShellExtClassFactory::LockServer(BOOL /*fLock*/)
{
    return E_NOTIMPL;
}
