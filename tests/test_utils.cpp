#include "test_utils.h"
#include <QtTest/QtTest>

#include "../src/utils/utils.h"

void Utils::testReadableFileSize() {
    QVERIFY(::readableFileSize(0) == "0B");
    QVERIFY(::readableFileSize(1024) == "1024B");
    QVERIFY(::readableFileSize(1025) == "1KB");
    QVERIFY(::readableFileSize(1<<20) == "1024KB");
    QVERIFY(::readableFileSize(1L<<30) == "1024MB");
    QVERIFY(::readableFileSize(1L<<40) == "1024GB");
}

void Utils::testReadableFileSizeV2() {
    QVERIFY(::readableFileSizeV2(0) == "0 B");
    QVERIFY(::readableFileSizeV2(1) == "1B");
    QVERIFY(::readableFileSizeV2(1<<10) == "1.00K");
    QVERIFY(::readableFileSizeV2(1024 + 512) == "1.50K");
    QVERIFY(::readableFileSizeV2(1<<20) == "1.00M");
    QVERIFY(::readableFileSizeV2(1L<<30) == "1.00G");
    QVERIFY(::readableFileSizeV2(1L<<40) == "1.00T");
    QVERIFY(::readableFileSizeV2(1024 + (1L<<40)) == "1.00T");
    QVERIFY(::readableFileSizeV2(1L<<50) == "1.00P");
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

    QVERIFY(urla.scheme() == urlb.scheme());
    QVERIFY(urla.host() == urlb.host());

    Q_FOREACH (const QString& key, params.keys()) {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
        QString encoded_key = QUrl::toPercentEncoding(key);
        QString encoded_value = QUrl::toPercentEncoding(params[encoded_key]);
        QUrlQuery query = QUrlQuery(urlb.query());
        QVERIFY(query.queryItemValue(encoded_key, QUrl::FullyEncoded) == encoded_value);
#else
        QVERIFY(urlb.queryItemValue(key) == params[key]);
#endif
    }
}

QTEST_APPLESS_MAIN(Utils)
