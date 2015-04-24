#ifndef TESTS_SERVER_INFO_H
#define TESTS_SERVER_INFO_H
#include <QObject>

class ServerInfoTest : public QObject {
    Q_OBJECT
public:
    virtual ~ServerInfoTest() {};

private slots:
    void testFeature();
    void testVersion();
};

#endif // TESTS_SERVER_INFO_H

