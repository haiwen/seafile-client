#include "test_file-utils.h"
#include <QtTest/QtTest>

#include "../src/utils/file-utils.h"

void FileUtils::getParentPath() {
    using ::getParentPath;
    QCOMPARE(getParentPath("/"), QString("/"));
    QCOMPARE(getParentPath("//"), QString("/"));
    QCOMPARE(getParentPath("/usr"), QString("/"));
    QCOMPARE(getParentPath("/usr/"), QString("/"));
    QCOMPARE(getParentPath("/usr/bin"), QString("/usr"));
    QCOMPARE(getParentPath("/usr/bin/"), QString("/usr"));
    QCOMPARE(getParentPath("/usr.complicate"), QString("/"));
    QCOMPARE(getParentPath("/usr/bin.pdf"), QString("/usr"));
    QCOMPARE(getParentPath(QString::fromUtf8("/usr/測試")), QString::fromUtf8("/usr"));
    QCOMPARE(getParentPath(QString::fromUtf8("/√∆/測試")), QString::fromUtf8("/√∆"));
}

void FileUtils::getBaseName() {
    using ::getBaseName;
    QCOMPARE(getBaseName("/"), QString("/"));
    QCOMPARE(getBaseName("//"), QString("/"));
    QCOMPARE(getBaseName("/usr"), QString("usr"));
    QCOMPARE(getBaseName("/usr/"), QString("usr"));
    QCOMPARE(getBaseName("/usr/bin"), QString("bin"));
    QCOMPARE(getBaseName("/usr/bin/"), QString("bin"));
    QCOMPARE(getBaseName("/usr.complicate"), QString("usr.complicate"));
    QCOMPARE(getBaseName("/usr/bin.pdf"), QString("bin.pdf"));
    QCOMPARE(getBaseName(QString::fromUtf8("/usr/測試")), QString::fromUtf8("測試"));
    QCOMPARE(getBaseName(QString::fromUtf8("/√∆/測試")), QString::fromUtf8("測試"));
}

void FileUtils::expandUser() {
    using::expandUser;
    QCOMPARE(expandUser("~"), QDir::homePath() + "/");
    QCOMPARE(expandUser("~/"), QDir::homePath() + "/");
    QCOMPARE(expandUser("~//testuser"), QDir::homePath() + "//testuser");
    QCOMPARE(expandUser("~/testuser"), QDir::home().filePath("testuser"));
    QCOMPARE(expandUser("~/testuser/test1/test2"), QDir::home().filePath("testuser/test1/test2"));
    QCOMPARE(expandUser("~testuser"), QFileInfo(QDir::homePath()).dir().filePath("testuser") + "/");
    QCOMPARE(expandUser("~testuser//"), QFileInfo(QDir::homePath()).dir().filePath("testuser") + "//");
    QCOMPARE(expandUser("~testuser/test1/test2/"), QFileInfo(QDir::homePath()).dir().filePath("testuser/test1/test2") + "/");
}

QTEST_APPLESS_MAIN(FileUtils)

