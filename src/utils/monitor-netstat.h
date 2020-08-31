#ifndef MONITOR_NETSTAT_H
#define MONITOR_NETSTAT_H
#include <QtGlobal>
#include <QObject>


class MonitorNetStatWorker : public QObject {
	Q_OBJECT
public:
	MonitorNetStatWorker();
	~MonitorNetStatWorker();
public slots:
	void process();
signals:
	void routerTableChanged();

};

#endif // MONITOR_NETSTAT_H