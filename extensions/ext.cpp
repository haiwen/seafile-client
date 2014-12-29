
#include "ext-common.h"
#include "class-factory.h"
#include "guids.h"
#include "log.h"

#include "shell-ext.h"

volatile LONG       g_cRefThisDll = 0;              ///< reference count of this DLL.
HINSTANCE           g_hmodThisDll = NULL;           ///< handle to this DLL itself.
int                 g_cAprInit = 0;
DWORD               g_langid;
DWORD               g_langTimeout = 0;
HINSTANCE           g_hResInst = NULL;

extern "C" int APIENTRY
DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /* lpReserved */)
{
    // NOTE: Do *NOT* call apr_initialize() or apr_terminate() here in DllMain(),
    // because those functions call LoadLibrary() indirectly through malloc().
    // And LoadLibrary() inside DllMain() is not allowed and can lead to unexpected
    // behavior and even may create dependency loops in the dll load order.
    Logger::debug("DllMain called");
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        if (g_hmodThisDll == NULL)
        {
            // g_csGlobalCOMGuard.Init();
        }

        // Extension DLL one-time initialization
        g_hmodThisDll = hInstance;
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        // do not clean up memory here:
        // if an application doesn't release all COM objects
        // but still unloads the dll, cleaning up ourselves
        // will lead to crashes.
        // better to leak some memory than to crash other apps.
        // sometimes an application doesn't release all COM objects
        // but still unloads the dll.
        // in that case, we do it ourselves

        // g_csGlobalCOMGuard.Term();
    }
    return 1;   // ok
}

STDAPI DllCanUnloadNow(void)
{
    return (g_cRefThisDll == 0 ? S_OK : S_FALSE);
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppvOut)
{
    if (ppvOut == 0 )
        return E_POINTER;
    *ppvOut = NULL;

    if (IsEqualIID(rclsid, CLSID_SEAFILE_SHELLEXT)) {
        // apr_initialize();
        // svn_dso_initialize2();
        // g_SVNAdminDir.Init();
        // g_cAprInit++;

        CShellExtClassFactory *pcf = new CShellExtClassFactory;
        const HRESULT hr = pcf->QueryInterface(riid, ppvOut);
        if(FAILED(hr))
            delete pcf;
        return hr;
    }

    return CLASS_E_CLASSNOTAVAILABLE;
}
