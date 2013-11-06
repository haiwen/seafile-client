## C++ Coding Style

Refer to `coding-style.md`

## I18N

### update the .ts files

        make update-ts


### Add a new language

1. add the language to the LANGUAGES variable in `CMakeLists.txt`

        SET(LANGUAGES zh_CN xx_YY)

2. make update-ts

3. add a line to `seafile-client.qrc`

        <file>i18n/seafile_xx_YY.qm</file>

### Add a New Language Contributed by Others

1. add the language to the LANGUAGES variable in `CMakeLists.txt`

        SET(LANGUAGES zh_CN xx_YY)

2. add a line to `seafile-client.qrc`

        <file>i18n/seafile_xx_YY.qm</file>

