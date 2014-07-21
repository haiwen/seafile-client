#ifndef TRAYNOTIFICATIONWIDGET_H
#define TRAYNOTIFICATIONWIDGET_H

#include <QWidget>

#include <QtGlobal>

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
#include <QtWidgets>
#else
#include <QtGui>
#endif

class TrayNotificationWidget : public QWidget
{
    Q_OBJECT
public:
    explicit TrayNotificationWidget(QPixmap pixmapIcon, QString headerText, QString messageText);

private:
    QTimer* timeout;

signals:
    void deleted(TrayNotificationWidget*);

public slots:
   void fadeOut();

};

#endif // TRAYNOTIFICATIONWIDGET_H
