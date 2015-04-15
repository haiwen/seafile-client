## seafile-client [![Build Status](https://secure.travis-ci.org/haiwen/seafile-client.svg?branch=master)](http://travis-ci.org/haiwen/seafile-client)


New version of Seafile desktop client.

## BUILD ##

### Prerequisites ###

- Qt4 (or Qt5)
- cmake
- [libsearpc](https://github.com/haiwen/libsearpc)
- [ccnet](https://github.com/haiwen/ccnet)
- [seafile](https://github.com/haiwen/seafile)
- [jansson](https://github.com/akheron/jansson)

### INSTALL ###

```
cmake .
make
make install
```

> Qt 4.8.0 or higher is required

### INSTALL with Qt5 ###

```
cmake -DUSE_QT5=on .
make
make install
```

> Qt 5.4.0 or higher is recommanded but not required

## Internationalization

You are welcome to add translation in your language.

### Contribute your translation

Please submit translations via Transifex: https://www.transifex.com/projects/p/seafile-client/

Steps:

1. Create a free account on Transifex (https://www.transifex.com/).
2. Send a request to join the language translation.
3. After accepted by the project maintainer, then you can translate online.
