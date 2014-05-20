## seafile-client ##

New version of Seafile desktop client.

## BUILD ##

### Prerequisites ###

- Qt4
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

## Internationalization

You are welcome to add translation in your language.

### Install Qt Linguist

First you need to install **Qt Linguist**:

- Windows/MacOS: download it from [here](http://qt-apps.org/content/show.php?content=89360)
- Linux users:

    `sudo apt-get install libqt4-dev`

### Do the translation

For example, if you like to add Russian translation.

1. Make a copy of an existing translation file in `i18n/` folder, like `i18n/seafile_zh_CN.ts`, and name it `i18n/seafile_ru_RU.ts`.
2. Start Qt Linguist
3. Open the file `i18n/seafile_ru_RU.ts` in Qt Linguist, and translate it entry by entry.

### Contribute your translation

Please submit translations via Transifex: https://www.transifex.com/projects/p/seafile-client/

Steps:

1. Create a free account on Transifex (https://www.transifex.com/).
2. Send a request to join the language translation.
3. After accepted by the project maintainer, then you can upload your file or translate online.
