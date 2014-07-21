#include "traynotificationmanager.h"

TrayNotificationManager::TrayNotificationManager(QObject *parent)
{
    notificationWidgets = new QList<TrayNotificationWidget*>();
    QDesktopWidget* desktopWidget = QApplication::desktop();
    QRect clientRect = desktopWidget->availableGeometry();
    m_maxTrayNotificationWidgets = 4;
    m_width = 320;
    m_height = 150;
    m_onScreenCount = 0;
#if defined(Q_OS_MAC)
    m_startX = clientRect.width() - m_width;
    m_startY = 10;
    m_up = false;
#endif

#if defined(Q_OS_LINUX)
    m_startX = clientRect.width() - m_width;
    m_startY = 10;
    m_up = false;
#endif

#if defined(Q_OS_WIN)
    m_startX = clientRect.width() - m_width;
    m_startY = clientRect.height() - m_height;
    m_up = true;
#endif

    m_deltaX = 0;
    m_deltaY = 0;
}

TrayNotificationManager::~TrayNotificationManager()
{
    notificationWidgets->clear();
    delete notificationWidgets;
}

void TrayNotificationManager::setMaxTrayNotificationWidgets(int max)
{
    this->m_maxTrayNotificationWidgets = max;
}

void TrayNotificationManager::append(TrayNotificationWidget* widget)
{
    connect(widget, SIGNAL(deleted(TrayNotificationWidget*)), this, SLOT(removeFirst(TrayNotificationWidget*)));
    qDebug() << "Count: " + QVariant(notificationWidgets->count()).toString();
    if(notificationWidgets->count() < m_maxTrayNotificationWidgets)
    {
        qDebug() << "Before: " + QVariant(m_deltaY).toString();

        if(notificationWidgets->count() > 0)
        {
            if(m_up)
                m_deltaY += -100;
            else
                m_deltaY += 100;
        }

        if(notificationWidgets->count() == 0)
            m_deltaY = 0;

        qDebug() << "After: " + QVariant(m_deltaY).toString();
    }
    else
    {
        m_deltaY = 0;
    }

    widget->setGeometry(m_startX + m_deltaX, m_startY + m_deltaY, widget->size().width(), widget->size().height());
    notificationWidgets->append(widget);
}

void TrayNotificationManager::removeFirst(TrayNotificationWidget *widget)
{
    int i = notificationWidgets->indexOf(widget);
    qDebug() << "Count: " + QVariant(notificationWidgets->count()).toString();
    qDebug() << "Index: " + QVariant(i).toString();

    if(notificationWidgets->count() > 0)
    {
        notificationWidgets->takeAt(i)->deleteLater();
        qDebug() << "Removing TrayNotificationWidget";
    }
}

void TrayNotificationManager::clear()
{
    for(int i = 0; i < notificationWidgets->count(); i++)
    {
        delete notificationWidgets->takeAt(i);
        qDebug() << "Removing TrayNotificationWidget";
    }
}
