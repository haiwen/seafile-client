#ifndef TESTS_STL_H
#define TESTS_STL_H
#include <QObject>

class STLTest : public QObject {
    Q_OBJECT
public:
    virtual ~STLTest() {};

private slots:
    void testBufferArry();
    void testWBufferArry();
};

#endif // TESTS_STL_H

