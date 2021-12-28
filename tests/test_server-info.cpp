#include "test_server-info.h"
#include <QSet>
#include <QtTest/QtTest>
#include <QtGlobal>

#include "../src/api/server-info.h"

namespace {
template <typename T, template<typename> class C>
QSet<T> toQSet(const C<T> &container)
{
  return QSet<T>(container.begin(), container.end());
}
} // anonymous namespace

void ServerInfoTest::testFeature() {
    ServerInfo info1;
    QString feature1 = "file-search,office-preview,seafile-pro";
    info1.parseFeatureFromStrings(feature1.split(','));
    QVERIFY(info1.proEdition);
    QVERIFY(info1.fileSearch);
    QVERIFY(info1.officePreview);
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    QSet<QString> info1_set = toQSet(feature1.split(','));
    QSet<QString> info1b_set = toQSet(info1.getFeatureStrings());
#else
    QSet<QString> info1_set = feature1.split(',').toSet();
    QSet<QString> info1b_set = info1.getFeatureStrings().toSet();
#endif
    QCOMPARE(info1_set, info1b_set);

    ServerInfo info2;
    QString feature2 = "file-search,seafile-pro";
    info2.parseFeatureFromStrings(feature2.split(','));
    QVERIFY(info2.proEdition);
    QVERIFY(info2.fileSearch);
    QVERIFY(!info2.officePreview);
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    QSet<QString> info2_set = toQSet(feature2.split(','));
    QSet<QString> info2b_set = toQSet(info2.getFeatureStrings());
#else
    QSet<QString> info2_set = feature2.split(',').toSet();
    QSet<QString> info2b_set = info2.getFeatureStrings().toSet();
#endif
    QCOMPARE(info2_set, info2b_set);

    ServerInfo info3;
    QString feature3 = "file-search,office-preview,seafile-pro";
    info3.parseFeatureFromStrings(feature3.split(','));
    QVERIFY(info3.proEdition);
    QVERIFY(info3.fileSearch);
    QVERIFY(info3.officePreview);
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    QSet<QString> info3_set = toQSet(feature3.split(','));
    QSet<QString> info3b_set = toQSet(info3.getFeatureStrings());
#else
    QSet<QString> info3_set = feature3.split(',').toSet();
    QSet<QString> info3b_set = info3.getFeatureStrings().toSet();
#endif
    QCOMPARE(info3_set, info3b_set);

    info3.parseFeatureFromString("office-preview", false);
    QVERIFY(!info3.officePreview);
}

void ServerInfoTest::testVersion() {
    ServerInfo info;
    QString version = "1.2.4";
    info.parseVersionFromString(version);
    QCOMPARE(info.majorVersion, 1u);
    QCOMPARE(info.minorVersion, 2u);
    QCOMPARE(info.patchVersion, 4u);
    QCOMPARE(info.getVersionString(), version);
}

QTEST_APPLESS_MAIN(ServerInfoTest)
