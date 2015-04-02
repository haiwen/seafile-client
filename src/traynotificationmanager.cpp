#include "traynotificationmanager.h"

TrayNotificationManager::TrayNotificationManager(QObject *parent)
  : notificationWidgets(new QList<TrayNotificationWidget*>()), QObject(parent)
{
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

#if defined(Q_OS_WIN32)
    m_startX = clientRect.width() - m_width;
    m_startY = clientRect.height() - m_height;
    m_up = true;
#endif

    m_deltaX = 0;
    m_deltaY = 0;
}

TrayNotificationManager::~TrayNotificationManager()
{
    // call delete and remove all remaining widgets in notificationWidgets
    clear();

    // then delete qlist notificationWidgets
    delete notificationWidgets;
}

void TrayNotificationManager::setMaxTrayNotificationWidgets(int max)
{
    this->m_maxTrayNotificationWidgets = max;
}

void TrayNotificationManager::append(TrayNotificationWidget* widget)
{
    connect(widget, SIGNAL(deleted()), this, SLOT(removeWidget()));
    if (notificationWidgets->count() < m_maxTrayNotificationWidgets)
    {
        if (!notificationWidgets->empty())
        {
            if(m_up)
                m_deltaY += -100;
            else
                m_deltaY += 100;
        } else
        {
            m_deltaY = 0;
        }
    }
    else
    {
        m_deltaY = 0;
    }

    widget->setGeometry(m_startX + m_deltaX, m_startY + m_deltaY, widget->size().width(), widget->size().height());
    notificationWidgets->append(widget);
}

void TrayNotificationManager::removeWidget()
{
    TrayNotificationWidget *widget = qobject_cast<TrayNotificationWidget*>(sender());

    if (widget == NULL)
    {
        return;
    }

    int i = notificationWidgets->indexOf(widget);

    if (i != -1)
    {
        notificationWidgets->takeAt(i)->deleteLater();
    }
}

void TrayNotificationManager::clear()
{
    // call operator delete on all items in list notificationWidgets
    qDeleteAll(*notificationWidgets);

    // remove all items from it since qDeleteAll don't do it for us
    notificationWidgets->clear();
}
