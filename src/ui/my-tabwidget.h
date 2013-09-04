#ifndef SEAFILE_CLIENT_MY_TABWIDGET_H
#define SEAFILE_CLIENT_MY_TABWIDGET_H

#include <QTabWidget>

/**
 * A subclass of QTabWidget to make the text horizontal when tabs are in the
 * west.
 *
 * See http://www.qtcentre.org/threads/13293-QTabWidget-customization
 */
class MyTabWidget: public QTabWidget {
    Q_OBJECT

public:
    MyTabWidget(QWidget *parent=0);

private:
    Q_DISABLE_COPY(MyTabWidget)
};

#endif // SEAFILE_CLIENT_MY_TABWIDGET_H
