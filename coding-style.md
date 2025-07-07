## C++ Coding Style

Mainly borrowed from [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)

### Naming Conventions

#### Member Variables

Lower case words separated by underscores, and ends with an underscore, e.g. `repos_list_`, `context_menu_`

#### Variables in Qt UI Files

Camel case starts with a "m", e.g. `mUserNameText`, `mServerAddr`

#### Functions

Camel case, e.g. `showRepos`

#### Setter and Getter

* Setter: `setRepoName()`
* Getter: `repoName()`

#### Constants

Camel case starts with "k", e.g. :

```cpp
const int kRepoRefreshInterval = 1000;
const char *kDefaultName = "seafile";
```

Use constant variables instead of macros to define constants.

### Invoking Functions

* Constant function parameters must be passed by object reference
* Do not use `this->` when invoking member functions

### Others

* No source file scope static variable/function, use anonymous namespace
* Use forward declarations when possible, instead of including unnecessary header files
* Never use exceptions
