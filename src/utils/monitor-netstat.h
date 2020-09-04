#ifndef MONITOR_NETSTAT_H
#define MONITOR_NETSTAT_H
#include <QtGlobal>
#include <QObject>

#include <windows.h>
#include <winsock2.h>
#include <netioapi.h>

#include "utils/singleton.h"


class MonitorNetStatWorker : public QObject {
    Q_OBJECT
    SINGLETON_DEFINE(MonitorNetStatWorker)
public:
    MonitorNetStatWorker();
    ~MonitorNetStatWorker();

signals:
    void routerTableChanged();

private:

};

#endif // MONITOR_NETSTAT_H