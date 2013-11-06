## C++ Coding Style

Mainly borrowed from [google c++ style coding style](http://google-styleguide.googlecode.com/svn/trunk/cppguide.xml)

### Naming Conventions

#### Member Vairables

Lower case words seprated by underscores, and ends with an underscore, e.g. `repos_list_`, `context_menu_`

#### Variables in Qt Ui Files

Camel case starts with a "m", e.g. `mUserNameText`, `mServerAddr`

#### Functions

Camel case, e.g. `showRepos`

#### Setter and Getter

- setter: `setRepoName()`
- getter: `repoName()`

#### Constants

Camel case starts with "k", e.g. :

    const int kRepoRefreshInterval = 1000;
    const char *kDefaultName = "seafile";

Use constants variables instead of macros to define constants.

### Invoking functions

- constant function parameter must be passed by object reference
- No `this->` when invoking member functions.

### Others

- no source file scope static variable/function, use anonymous namespace
- use forward declaration when possible, instead of including unnecessary header files
- Never use exceptions

