#ifndef SEAFILE_EXT_SHELL_EXT_CLASS_FACTORY_H
#define SEAFILE_EXT_SHELL_EXT_CLASS_FACTORY_H

/**
 * Class factory's main responsibility is implemented in its `CreateInstance`
 * member function, which creates instances of the required shell extension
 * interface, such as IContextMenu.
 *
 * Class factory object is created by `DllGetClassObject` method for this
 * extension.
 */
class ShellExtClassFactory : public IClassFactory
{
protected:
    ULONG m_cRef;

public:
    ShellExtClassFactory();
    virtual ~ShellExtClassFactory();

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
