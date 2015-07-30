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

const int kWorktreeCacheExpireMSecs = 3 * 1000;

} // namespace

std::unique_ptr<seafile::RepoInfoList> ShellExt::repos_cache_;
uint64_t ShellExt::cache_ts_;

// *********************** ShellExt *************************
ShellExt::ShellExt(seafile::RepoInfo::Status status)
  : main_menu_(0),
    index_(0),
    first_(0),
    last_(0)
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
    seafile::utils::MutexLocker lock(&repos_cache_mutex_);

    uint64_t now = utils::currentMSecsSinceEpoch();
    if (repos_cache_ && now < cache_ts_ + kWorktreeCacheExpireMSecs) {
        *wts = *(repos_cache_.get());
        // seaf_ext_log("use cached repos list!");
        return true;
    }

    // no cached worktree list, send request to seafile client
    seafile::ListReposCommand cmd;
    seafile::RepoInfoList repos;
    if (!cmd.sendAndWait(&repos)) {
        // seaf_ext_log("ListReposCommand returned false!");
        return false;
    }

    cache_ts_ = utils::currentMSecsSinceEpoch();
    repos_cache_.reset(new seafile::RepoInfoList(repos));

    *wts = repos;
    return true;
}

bool ShellExt::pathInRepo(const std::string& path,
                          std::string *path_in_repo,
                          seafile::RepoInfo *repo)
{
    seafile::RepoInfoList repos;
    if (!getReposList(&repos)) {
        return false;
    }
    std::string p = utils::normalizedPath(path);

    for (size_t i = 0; i < repos.size(); i++) {
        std::string wt = repos[i].worktree;
        // seaf_ext_log ("work tree is %s, path is %s\n", wt.c_str(), p.c_str());
        if (p.size() >= wt.size() && p.substr(0, wt.size()) == wt) {
            if (p.size() > wt.size() && p[wt.size()] != '/') {
                continue;
            }
            *path_in_repo = p.substr(wt.size(), p.size() - wt.size());
            if (repo) {
                *repo = repos[i];
            }
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

seafile::RepoInfo::Status
ShellExt::getRepoFileStatus(const std::string& repo_id,
                            const std::string& path_in_repo,
                            bool isdir)
{
    // TODO: get the files under the same folder in a single command to reduce overhead
    seafile::GetFileStatusCommand cmd(repo_id, path_in_repo, isdir);
    seafile::RepoInfo::Status status;
    if (!cmd.sendAndWait(&status)) {
        return seafile::RepoInfo::NoStatus;
    }

    return status;
}
