#include "test_utils.h"
#include <QtTest/QtTest>

#include "../src/utils/utils.h"

void Utils::testReadableFileSize() {
    QCOMPARE(::readableFileSize(0), QString("0B"));
    QCOMPARE(::readableFileSize(1024), QString("1024B"));
    QCOMPARE(::readableFileSize(1025), QString("1KB"));
    QCOMPARE(::readableFileSize(1<<20), QString("1024KB"));
    QCOMPARE(::readableFileSize(1L<<30), QString("1024MB"));
    QCOMPARE(::readableFileSize(1L<<40), QString("1024GB"));
}

void Utils::testReadableFileSizeV2() {
    QCOMPARE(::readableFileSizeV2(0), QString("0 B"));
    QCOMPARE(::readableFileSizeV2(1), QString("1B"));
    QCOMPARE(::readableFileSizeV2(1<<10), QString("1.00K"));
    QCOMPARE(::readableFileSizeV2(1024 + 512), QString("1.50K"));
    QCOMPARE(::readableFileSizeV2(1<<20), QString("1.00M"));
    QCOMPARE(::readableFileSizeV2(1L<<30), QString("1.00G"));
    QCOMPARE(::readableFileSizeV2(1L<<40), QString("1.00T"));
    QCOMPARE(::readableFileSizeV2(1024 + (1L<<40)), QString("1.00T"));
    QCOMPARE(::readableFileSizeV2(1L<<50), QString("1.00P"));
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

QTEST_APPLESS_MAIN(Utils)
