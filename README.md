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

Just fork seafile-client and send us a pull request, and we'll check and merge your translation.

### Update the translation

Some time later after the your initial translation is merged, new content need to be translated as seafile-client source code evolves. To update the translation, just open the `.ts` file with Qt Linguist, and translate all unfinished items.
