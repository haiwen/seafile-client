## seafile-client [![Build Status](https://secure.travis-ci.org/haiwen/seafile-client.svg?branch=master)](http://travis-ci.org/haiwen/seafile-client)

[Seafile](https://seafile.com) desktop client.

## Build Seafile Client for Mac And Linux ##

### Prerequisites ###

- Qt5
- cmake
- [libsearpc](https://github.com/haiwen/libsearpc)
- [seafile](https://github.com/haiwen/seafile)

### INSTALL ###

```
cmake .
make
make install
```

> Qt 5.2 or higher is required

Ubuntu users can install seafile client from this [PPA](https://code.launchpad.net/~seafile/+archive/ubuntu/seafile-client)

### INSTALL with Qt5 ###

```
cmake .
make
make install
```

> Qt 5.5 or higher is recommanded but not required

## Build Seafile Client for Windows

### The development environment

Before compiling and developing, you need to install and configure the development environment

1. Microsoft Visual Studio 2019

2. Package manager tools: [vcpkg](https://docs.microsoft.com/en-us/cpp/build/vcpkg?view=vs-2019)

### Installing third-party libraries

```bash
glib, curl, openssl, libevent, jansson, sqlite3, pthreads, getopt-win32

Install curl：`vcpkg.exe install curl[core,openssl]:x64-windows`
```

Install and configure Qt:

1. Search and install Qt Visual Studio tools in the Visual Studio plugin repository
2. Install Qt, choose qt-opensource-windows-x86-5.13.1.exe ([QT download link](http://download.qt.io/archive/qt/)).
3. Set Qt Visual Studio Tools. The setting is to tell Visual Studio 2019 Qt installation directory. Set Qt Version in Extension->Qt VS Tools-> Qt Options in Visual Studio 2019. Here you need to select the bin directory of Qt. The path of the parent directory. After configuring the Qt installation directory, you also need to set the Qt version used in the project properties of seafile-client. Select properties in the right-click menu of the project, and then select "Qt Project Settings" in the properties dialog box, and select the Version name set in Qt VS Tools just now for Qt Installation.

### Build and run

The three source directories of libsearpc, seafile and seafile-client need to be placed under the same parent directory. You need to select the x64 target architecture when compiling.

1. Build Seafile-client

Use git to clone the code and then use vs to compile

```bash
git clone git@github.com:haiwen/seafile-client.git
cd seafile-client
devenv seafile-client.sln /build "Release|x64"
```

## Internationalization

You are welcome to add translation in your language.

### Contribute your translation

Please submit translations via Transifex: https://www.transifex.com/projects/p/seafile-client/

Steps:

1. Create a free account on Transifex (https://www.transifex.com/).
2. Send a request to join the language translation.
3. After accepted by the project maintainer, then you can translate online.
