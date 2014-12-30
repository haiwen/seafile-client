#include "test_file-utils.h"
#include <QtTest/QtTest>

#include "../src/utils/file-utils.h"

void FileUtils::getParentPath() {
    using ::getParentPath;
    QVERIFY(getParentPath("/") == "/");
    QVERIFY(getParentPath("//") == "/");
    QVERIFY(getParentPath("/usr") == "/");
    QVERIFY(getParentPath("/usr/") == "/");
    QVERIFY(getParentPath("/usr/bin") == "/usr");
    QVERIFY(getParentPath("/usr/bin/") == "/usr");
    QVERIFY(getParentPath("/usr.complicate") == "/");
    QVERIFY(getParentPath("/usr/bin.pdf") == "/usr");
    QVERIFY(getParentPath(QString::fromUtf8("/usr/測試")) == QString::fromUtf8("/usr"));
    QVERIFY(getParentPath(QString::fromUtf8("/√∆/測試")) == QString::fromUtf8("/√∆"));
    //XFAIL:
}

void FileUtils::getBaseName() {
    using ::getBaseName;
    QVERIFY(getBaseName("/") == "/");
    QVERIFY(getBaseName("//") == "/");
    QVERIFY(getBaseName("/usr") == "usr");
    QVERIFY(getBaseName("/usr/") == "usr");
    QVERIFY(getBaseName("/usr/bin") == "bin");
    QVERIFY(getBaseName("/usr/bin/") == "bin");
    QVERIFY(getBaseName("/usr.complicate") == "usr.complicate");
    QVERIFY(getBaseName("/usr/bin.pdf") == "bin.pdf");
    QVERIFY(getBaseName(QString::fromUtf8("/usr/測試")) == QString::fromUtf8("測試"));
    QVERIFY(getBaseName(QString::fromUtf8("/√∆/測試")) == QString::fromUtf8("測試"));
    //XFAIL:
}

QTEST_APPLESS_MAIN(FileUtils)

