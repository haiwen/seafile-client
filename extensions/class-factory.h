#ifndef SEAFILE_EXT_SHELL_EXT_CLASS_FACTORY_H
#define SEAFILE_EXT_SHELL_EXT_CLASS_FACTORY_H

class CShellExtClassFactory : public IClassFactory
{
protected:
    ULONG m_cRef;

public:
    CShellExtClassFactory();
    virtual ~CShellExtClassFactory();

    //@{
    /// IUnknown members
    STDMETHODIMP         QueryInterface(REFIID, LPVOID FAR *);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();
    //@}

    //@{
    /// IClassFactory members
    STDMETHODIMP      CreateInstance(LPUNKNOWN, REFIID, LPVOID FAR *);
    STDMETHODIMP      LockServer(BOOL);
    //@}
};

#endif // SEAFILE_EXT_SHELL_EXT_CLASS_FACTORY_H
