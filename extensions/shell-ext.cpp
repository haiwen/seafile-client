// Initialize GUIDs (should be done only and at-least once per DLL/EXE)
#include <initguid.h>
#include "guids.h"

#include "ext-common.h"
#include "ext-utils.h"
#include "commands.h"
#include "log.h"
#include "shell-ext.h"

namespace utils = seafile::utils;

namespace {

const int kWorktreeCacheExpireMSecs = 10 * 1000;

} // namespace

std::unique_ptr<seafile::RepoInfoList> ShellExt::repos_cache_;
uint64_t ShellExt::cache_ts_;

// *********************** ShellExt *************************
ShellExt::ShellExt(seafile::RepoInfo::Status status)
{
    m_cRef = 0L;
    InterlockedIncrement(&g_cRefThisDll);

    sub_menu_ = CreateMenu();
    next_active_item_ = 0;
    status_ = status;

    // INITCOMMONCONTROLSEX used = {
    //     sizeof(INITCOMMONCONTROLSEX),
    //         ICC_LISTVIEW_CLASSES | ICC_WIN95_CLASSES | ICC_BAR_CLASSES | ICC_USEREX_CLASSES
    // };
    // InitCommonControlsEx(&used);
}

ShellExt::~ShellExt()
{
    InterlockedDecrement(&g_cRefThisDll);
}

STDMETHODIMP ShellExt::QueryInterface(REFIID riid, LPVOID FAR *ppv)
{
    if (ppv == 0)
        return E_POINTER;

    *ppv = NULL;

    if (IsEqualIID(riid, IID_IShellExtInit) || IsEqualIID(riid, IID_IUnknown)) {
        *ppv = static_cast<LPSHELLEXTINIT>(this);
    }
    else if (IsEqualIID(riid, IID_IContextMenu)) {
        *ppv = static_cast<LPCONTEXTMENU>(this);
    }
    else if (IsEqualIID(riid, IID_IShellIconOverlayIdentifier)) {
        *ppv = static_cast<IShellIconOverlayIdentifier*>(this);
    }
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

STDMETHODIMP_(ULONG) ShellExt::AddRef()
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) ShellExt::Release()
{
    if (--m_cRef)
        return m_cRef;

    delete this;

    return 0L;
}

bool ShellExt::getReposList(seafile::RepoInfoList *wts)
{
    uint64_t now = utils::currentMSecsSinceEpoch();
    if (repos_cache_ && now < cache_ts_ + kWorktreeCacheExpireMSecs) {
        *wts = *(repos_cache_.get());
        return true;
    }

    // no cached worktree list, send request to seafile client
    seafile::ListReposCommand cmd;
    seafile::RepoInfoList repos;
    if (!cmd.sendAndWait(&repos)) {
        return false;
    }

    cache_ts_ = utils::currentMSecsSinceEpoch();
    repos_cache_.reset(new seafile::RepoInfoList(repos));

    *wts = repos;
    return true;
}

bool ShellExt::pathInRepo(const std::string path, std::string *path_in_repo)
{
    seafile::RepoInfoList repos;
    if (!getReposList(&repos)) {
        return false;
    }
    std::string p = utils::normalizedPath(path);

    for (size_t i = 0; i < repos.size(); i++) {
        std::string wt = repos[i].worktree;
        if (path.size() >= wt.size() && path.substr(0, wt.size()) == wt) {
            *path_in_repo = path.substr(wt.size(), path.size() - wt.size());
            return true;
        }
    }

    return false;
}

bool ShellExt::isRepoTopDir(const std::string& path)
{
    seafile::RepoInfoList repos;
    if (!getReposList(&repos)) {
        return false;
    }

    std::string p = utils::normalizedPath(path);
    for (size_t i = 0; i < repos.size(); i++) {
        std::string wt = repos[i].worktree;
        if (p == wt) {
            return true;
        }
    }

    return false;
}

seafile::RepoInfo ShellExt::getRepoInfoByPath(const std::string& path)
{
    seafile::RepoInfoList repos;
    if (!getReposList(&repos)) {
        return seafile::RepoInfo();
    }

    std::string p = utils::normalizedPath(path);
    for (size_t i = 0; i < repos.size(); i++) {
        std::string wt = repos[i].worktree;
        if (p == wt) {
            return repos[i];
        }
    }

    return seafile::RepoInfo();
}
