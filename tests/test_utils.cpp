#include "test_utils.h"
#include <QtTest/QtTest>

#include "../src/utils/utils.h"

void Utils::testReadableFileSize() {
    QCOMPARE(::readableFileSize(0), QString("0B"));
    QCOMPARE(::readableFileSize(1000), QString("1KB"));
    QCOMPARE(::readableFileSize(1500), QString("2KB"));
    QCOMPARE(::readableFileSize(1000000), QString("1.0MB"));
    QCOMPARE(::readableFileSize(1230000), QString("1.2MB"));
    QCOMPARE(::readableFileSize(1000 * 1000 * 1000), QString("1.0GB"));
    QCOMPARE(::readableFileSize(1100 * 1000 * 1000), QString("1.1GB"));
}


void Utils::testIncludeUrlParams() {
    QUrl urla(QString("http://example.com"));

    QHash<QString, QString> params;
    params.insert("simple", "c");
    params.insert("withspecial", "a?b");
    params.insert("withspace", "a b");
    params.insert("username", "a123fx b");
    params.insert("password", "!@#+-$%^12&*()qweqesaf\"';`~");
    params.insert("withplus", "a+b");

    QUrl urlb = ::includeQueryParams(urla, params);

    QCOMPARE(urla.scheme(), urlb.scheme());
    QCOMPARE(urla.host(), urlb.host());

    Q_FOREACH (const QString& key, params.keys()) {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
        QString encoded_key = QUrl::toPercentEncoding(key);
        QString encoded_value = QUrl::toPercentEncoding(params[encoded_key]);
        QUrlQuery query = QUrlQuery(urlb.query());
        QCOMPARE(query.queryItemValue(encoded_key, QUrl::FullyEncoded), encoded_value);
#else
        QCOMPARE(urlb.queryItemValue(key), params[key]);
#endif
    }
}

void Utils::testDigitalCompare() {
    QCOMPARE(::digitalCompare("9", "9"), 0);
    QCOMPARE(::digitalCompare("aa9aa", "aa9aa"), 0);
    QCOMPARE(::digitalCompare("99a99", "99a99"), 0);
    QCOMPARE(::digitalCompare("9", "11"), -2);
    QCOMPARE(::digitalCompare("1.9", "1.11"), -2);
    QCOMPARE(::digitalCompare("1 9", "1 11"), -2);
    QCOMPARE(::digitalCompare("1abc1", "1abc1.abc"), -1);
    QCOMPARE(::digitalCompare("1.1.1.1.9", "1.1.1.1.11"), -2);
    QCOMPARE(::digitalCompare("a9", "a11"), -2);
    QCOMPARE(::digitalCompare("a9aaa", "a11aaa"), -2);
    QCOMPARE(::digitalCompare("zzz9", "bbb11"), QString::compare("zzz9", "bbb11"));
    QCOMPARE(::digitalCompare("pp9p", "pa11a"), QString::compare("pp9p", "pa11a"));
}

QTEST_APPLESS_MAIN(Utils)
