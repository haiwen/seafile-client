#ifndef TRAYNOTIFICATIONWIDGET_H
#define TRAYNOTIFICATIONWIDGET_H

#include <QWidget>

#include <QtGlobal>

#include <QtWidgets>

class TrayNotificationWidget : public QWidget
{
    Q_OBJECT
public:
    explicit TrayNotificationWidget(QPixmap pixmapIcon, QString headerText, QString messageText);

private:
    QTimer* timeout;
signals:
    void deleted();

private slots:
    void fadeOut();
};

#endif // TRAYNOTIFICATIONWIDGET_H
