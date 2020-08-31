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
    OVERLAPPED overlap;
    DWORD ret;

    HANDLE h1 = NULL;
    overlap.hEvent = WSACreateEvent();

    while (1)
    {
        ret = NotifyRouteChange(&h1, &overlap);
        if (ret != ERROR_IO_PENDING) {
            qDebug("NotifyRouteChange error...%d\n", WSAGetLastError());
        }

        if (WaitForSingleObject(overlap.hEvent, INFINITE) == WAIT_OBJECT_0)
        {
            qWarning("IPv4 routing table has changed");
		    emit routerTableChanged();
        }
    }
    return;
}