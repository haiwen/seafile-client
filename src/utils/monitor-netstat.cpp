#include <QFile>

#include <winsock2.h>
#include <iphlpapi.h>

#include "utils/monitor-netstat.h"
#include "utils/utils-win.h"


MonitorNetStatWorker::MonitorNetStatWorker() {

}

MonitorNetStatWorker::~MonitorNetStatWorker() {

}

void MonitorNetStatWorker::process() {
    DWORD ret;

    while (1)
    {
        ret = NotifyRouteChange(NULL, NULL);
        if (ret != NO_ERROR) {
            qDebug("NotifyRouteChange error...%d\n", WSAGetLastError());
        } else {
            qWarning("IPv4 routing table has changed");
            emit routerTableChanged();
        }
    }
    return;
}
