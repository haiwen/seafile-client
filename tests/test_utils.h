#ifndef TESTS_UTILS_H
#define TESTS_UTILS_H
#include <QObject>

class Utils : public QObject {
    Q_OBJECT
public:
    virtual ~Utils() {};

private slots:
    void dummy();
    void readableFileSize();
    void readableFileSizeV2();
};

#endif // TESTS_UTILS_H

