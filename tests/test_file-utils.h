#ifndef TESTS_FILE_UTILS_H
#define TESTS_FILE_UTILS_H
#include <QObject>

class FileUtils : public QObject {
    Q_OBJECT
public:
    virtual ~FileUtils() {};

private slots:
    void getParentPath();
    void getBaseName();
    void expandUser();
};

#endif // TESTS_FILE_UTILS_H

